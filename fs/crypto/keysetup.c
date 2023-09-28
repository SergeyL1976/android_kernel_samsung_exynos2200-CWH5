// SPDX-License-Identifier: GPL-2.0
/*
 * Key setup facility for FS encryption support.
 *
 * Copyright (C) 2015, Google, Inc.
 *
 * Originally written by Michael Halcrow, Ildar Muslukhov, and Uday Savagaonkar.
 * Heavily modified since then.
 */

#include <crypto/skcipher.h>
#include <linux/key.h>
#include <linux/random.h>

#include "fscrypt_private.h"
#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
#include "sdp/sdp_crypto.h"
#endif

struct fscrypt_mode fscrypt_modes[] = {
	[FSCRYPT_MODE_AES_256_XTS] = {
		.friendly_name = "AES-256-XTS",
		.cipher_str = "xts(aes)",
		.keysize = 64,
		.security_strength = 32,
		.ivsize = 16,
		.blk_crypto_mode = BLK_ENCRYPTION_MODE_AES_256_XTS,
	},
	[FSCRYPT_MODE_AES_256_CTS] = {
		.friendly_name = "AES-256-CTS-CBC",
		.cipher_str = "cts(cbc(aes))",
		.keysize = 32,
		.security_strength = 32,
		.ivsize = 16,
	},
	[FSCRYPT_MODE_AES_128_CBC] = {
		.friendly_name = "AES-128-CBC-ESSIV",
		.cipher_str = "essiv(cbc(aes),sha256)",
		.keysize = 16,
		.security_strength = 16,
		.ivsize = 16,
		.blk_crypto_mode = BLK_ENCRYPTION_MODE_AES_128_CBC_ESSIV,
	},
	[FSCRYPT_MODE_AES_128_CTS] = {
		.friendly_name = "AES-128-CTS-CBC",
		.cipher_str = "cts(cbc(aes))",
		.keysize = 16,
		.security_strength = 16,
		.ivsize = 16,
	},
	[FSCRYPT_MODE_ADIANTUM] = {
		.friendly_name = "Adiantum",
		.cipher_str = "adiantum(xchacha12,aes)",
		.keysize = 32,
		.security_strength = 32,
		.ivsize = 32,
		.blk_crypto_mode = BLK_ENCRYPTION_MODE_ADIANTUM,
	},
};

#ifdef CONFIG_FSCRYPT_SDP
static int derive_fek(struct inode *inode,
		struct fscrypt_info *crypt_info,
		u8 *fek, u32 fek_len);
#endif

static DEFINE_MUTEX(fscrypt_mode_key_setup_mutex);

static struct fscrypt_mode *
select_encryption_mode(const union fscrypt_policy *policy,
		       const struct inode *inode)
{
	BUILD_BUG_ON(ARRAY_SIZE(fscrypt_modes) != FSCRYPT_MODE_MAX + 1);

	if (S_ISREG(inode->i_mode))
		return &fscrypt_modes[fscrypt_policy_contents_mode(policy)];

	if (S_ISDIR(inode->i_mode) || S_ISLNK(inode->i_mode))
		return &fscrypt_modes[fscrypt_policy_fnames_mode(policy)];

	WARN_ONCE(1, "fscrypt: filesystem tried to load encryption info for inode %lu, which is not encryptable (file type %d)\n",
		  inode->i_ino, (inode->i_mode & S_IFMT));
	return ERR_PTR(-EINVAL);
}

/* Create a symmetric cipher object for the given encryption mode and key */
static struct crypto_skcipher *
fscrypt_allocate_skcipher(struct fscrypt_mode *mode, const u8 *raw_key,
			  const struct inode *inode)
{
	struct crypto_skcipher *tfm;
	int err;

	tfm = crypto_alloc_skcipher(mode->cipher_str, 0, 0);
	if (IS_ERR(tfm)) {
		if (PTR_ERR(tfm) == -ENOENT) {
			fscrypt_warn(inode,
				     "Missing crypto API support for %s (API name: \"%s\")",
				     mode->friendly_name, mode->cipher_str);
			return ERR_PTR(-ENOPKG);
		}
		fscrypt_err(inode, "Error allocating '%s' transform: %ld",
			    mode->cipher_str, PTR_ERR(tfm));
		return tfm;
	}
	if (!xchg(&mode->logged_impl_name, 1)) {
		/*
		 * fscrypt performance can vary greatly depending on which
		 * crypto algorithm implementation is used.  Help people debug
		 * performance problems by logging the ->cra_driver_name the
		 * first time a mode is used.
		 */
		pr_info("fscrypt: %s using implementation \"%s\"\n",
			mode->friendly_name, crypto_skcipher_driver_name(tfm));
	}
	if (WARN_ON(crypto_skcipher_ivsize(tfm) != mode->ivsize)) {
		err = -EINVAL;
		goto err_free_tfm;
	}
	crypto_skcipher_set_flags(tfm, CRYPTO_TFM_REQ_FORBID_WEAK_KEYS);
	err = crypto_skcipher_setkey(tfm, raw_key, mode->keysize);
	if (err)
		goto err_free_tfm;

	return tfm;

err_free_tfm:
	crypto_free_skcipher(tfm);
	return ERR_PTR(err);
}

/*
 * Prepare the crypto transform object or blk-crypto key in @prep_key, given the
 * raw key, encryption mode, and flag indicating which encryption implementation
 * (fs-layer or blk-crypto) will be used.
 */
int fscrypt_prepare_key(struct fscrypt_prepared_key *prep_key,
			const u8 *raw_key, unsigned int raw_key_size,
			bool is_hw_wrapped, const struct fscrypt_info *ci)
{
	struct crypto_skcipher *tfm;

	if (fscrypt_using_inline_encryption(ci))
		return fscrypt_prepare_inline_crypt_key(prep_key,
				raw_key, raw_key_size, is_hw_wrapped, ci);

	if (WARN_ON(is_hw_wrapped || raw_key_size != ci->ci_mode->keysize))
		return -EINVAL;

	tfm = fscrypt_allocate_skcipher(ci->ci_mode, raw_key, ci->ci_inode);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);
	/*
	 * Pairs with the smp_load_acquire() in fscrypt_is_key_prepared().
	 * I.e., here we publish ->tfm with a RELEASE barrier so that
	 * concurrent tasks can ACQUIRE it.  Note that this concurrency is only
	 * possible for per-mode keys, not for per-file keys.
	 */
	smp_store_release(&prep_key->tfm, tfm);
	return 0;
}

/* Destroy a crypto transform object and/or blk-crypto key. */
void fscrypt_destroy_prepared_key(struct fscrypt_prepared_key *prep_key)
{
	crypto_free_skcipher(prep_key->tfm);
	fscrypt_destroy_inline_crypt_key(prep_key);
}

/* Given a per-file encryption key, set up the file's crypto transform object */
int fscrypt_set_per_file_enc_key(struct fscrypt_info *ci, const u8 *raw_key)
{
	ci->ci_owns_key = true;
	return fscrypt_prepare_key(&ci->ci_enc_key, raw_key,
				   ci->ci_mode->keysize,
				   false /*is_hw_wrapped*/, ci);
}

static int setup_per_mode_enc_key(struct fscrypt_info *ci,
				  struct fscrypt_master_key *mk,
				  struct fscrypt_prepared_key *keys,
				  u8 hkdf_context, bool include_fs_uuid)
{
	const struct inode *inode = ci->ci_inode;
	const struct super_block *sb = inode->i_sb;
	struct fscrypt_mode *mode = ci->ci_mode;
	const u8 mode_num = mode - fscrypt_modes;
	struct fscrypt_prepared_key *prep_key;
	u8 mode_key[FSCRYPT_MAX_KEY_SIZE];
	u8 hkdf_info[sizeof(mode_num) + sizeof(sb->s_uuid)];
	unsigned int hkdf_infolen = 0;
	int err;

	if (WARN_ON(mode_num > FSCRYPT_MODE_MAX))
		return -EINVAL;

	prep_key = &keys[mode_num];
	if (fscrypt_is_key_prepared(prep_key, ci)) {
		ci->ci_enc_key = *prep_key;
		return 0;
	}

	mutex_lock(&fscrypt_mode_key_setup_mutex);

	if (fscrypt_is_key_prepared(prep_key, ci))
		goto done_unlock;

	if (mk->mk_secret.is_hw_wrapped && S_ISREG(inode->i_mode)) {
		int i;

		if (!fscrypt_using_inline_encryption(ci)) {
			fscrypt_warn(ci->ci_inode,
				     "Hardware-wrapped keys require inline encryption (-o inlinecrypt)");
			err = -EINVAL;
			goto out_unlock;
		}
		for (i = 0; i <= FSCRYPT_MODE_MAX; i++) {
			if (fscrypt_is_key_prepared(&keys[i], ci)) {
				fscrypt_warn(ci->ci_inode,
					     "Each hardware-wrapped key can only be used with one encryption mode");
				err = -EINVAL;
				goto out_unlock;
			}
		}
		err = fscrypt_prepare_key(prep_key, mk->mk_secret.raw,
					  mk->mk_secret.size, true, ci);
		if (err)
			goto out_unlock;
	} else {
		BUILD_BUG_ON(sizeof(mode_num) != 1);
		BUILD_BUG_ON(sizeof(sb->s_uuid) != 16);
		BUILD_BUG_ON(sizeof(hkdf_info) != 17);
		hkdf_info[hkdf_infolen++] = mode_num;
		if (include_fs_uuid) {
			memcpy(&hkdf_info[hkdf_infolen], &sb->s_uuid,
				   sizeof(sb->s_uuid));
			hkdf_infolen += sizeof(sb->s_uuid);
		}
		err = fscrypt_hkdf_expand(&mk->mk_secret.hkdf,
					  hkdf_context, hkdf_info, hkdf_infolen,
					  mode_key, mode->keysize);
		if (err)
			goto out_unlock;
		err = fscrypt_prepare_key(prep_key, mode_key, mode->keysize,
					  false /*is_hw_wrapped*/, ci);
		memzero_explicit(mode_key, mode->keysize);
		if (err)
			goto out_unlock;
	}
done_unlock:
	ci->ci_enc_key = *prep_key;
	err = 0;
out_unlock:
	mutex_unlock(&fscrypt_mode_key_setup_mutex);
	return err;
}

/*
 * Derive a SipHash key from the given fscrypt master key and the given
 * application-specific information string.
 *
 * Note that the KDF produces a byte array, but the SipHash APIs expect the key
 * as a pair of 64-bit words.  Therefore, on big endian CPUs we have to do an
 * endianness swap in order to get the same results as on little endian CPUs.
 */
static int fscrypt_derive_siphash_key(const struct fscrypt_master_key *mk,
				      u8 context, const u8 *info,
				      unsigned int infolen, siphash_key_t *key)
{
	int err;

	err = fscrypt_hkdf_expand(&mk->mk_secret.hkdf, context, info, infolen,
				  (u8 *)key, sizeof(*key));
	if (err)
		return err;

	BUILD_BUG_ON(sizeof(*key) != 16);
	BUILD_BUG_ON(ARRAY_SIZE(key->key) != 2);
	le64_to_cpus(&key->key[0]);
	le64_to_cpus(&key->key[1]);
	return 0;
}

int fscrypt_derive_dirhash_key(struct fscrypt_info *ci,
			       const struct fscrypt_master_key *mk)
{
	int err;

	err = fscrypt_derive_siphash_key(mk, HKDF_CONTEXT_DIRHASH_KEY,
					 ci->ci_nonce, FSCRYPT_FILE_NONCE_SIZE,
					 &ci->ci_dirhash_key);
	if (err)
		return err;
	ci->ci_dirhash_key_initialized = true;
	return 0;
}

void fscrypt_hash_inode_number(struct fscrypt_info *ci,
			       const struct fscrypt_master_key *mk)
{
	WARN_ON(ci->ci_inode->i_ino == 0);
	WARN_ON(!mk->mk_ino_hash_key_initialized);

	ci->ci_hashed_ino = (u32)siphash_1u64(ci->ci_inode->i_ino,
					      &mk->mk_ino_hash_key);
}

static int fscrypt_setup_iv_ino_lblk_32_key(struct fscrypt_info *ci,
					    struct fscrypt_master_key *mk)
{
	int err;

	err = setup_per_mode_enc_key(ci, mk, mk->mk_iv_ino_lblk_32_keys,
				     HKDF_CONTEXT_IV_INO_LBLK_32_KEY, true);
	if (err)
		return err;

	/* pairs with smp_store_release() below */
	if (!smp_load_acquire(&mk->mk_ino_hash_key_initialized)) {

		mutex_lock(&fscrypt_mode_key_setup_mutex);

		if (mk->mk_ino_hash_key_initialized)
			goto unlock;

		err = fscrypt_derive_siphash_key(mk,
						 HKDF_CONTEXT_INODE_HASH_KEY,
						 NULL, 0, &mk->mk_ino_hash_key);
		if (err)
			goto unlock;
		/* pairs with smp_load_acquire() above */
		smp_store_release(&mk->mk_ino_hash_key_initialized, true);
unlock:
		mutex_unlock(&fscrypt_mode_key_setup_mutex);
		if (err)
			return err;
	}

	/*
	 * New inodes may not have an inode number assigned yet.
	 * Hashing their inode number is delayed until later.
	 */
	if (ci->ci_inode->i_ino)
		fscrypt_hash_inode_number(ci, mk);
	return 0;
}

static int fscrypt_setup_v2_file_key(struct fscrypt_info *ci,
				     struct fscrypt_master_key *mk,
				     bool need_dirhash_key)
{
	int err;

	if (mk->mk_secret.is_hw_wrapped &&
	    !(ci->ci_policy.v2.flags & (FSCRYPT_POLICY_FLAG_IV_INO_LBLK_64 |
					FSCRYPT_POLICY_FLAG_IV_INO_LBLK_32))) {
		fscrypt_warn(ci->ci_inode,
			     "Hardware-wrapped keys are only supported with IV_INO_LBLK policies");
		return -EINVAL;
	}

#ifdef CONFIG_FSCRYPT_SDP
	if (fscrypt_sdp_is_classified(ci)) {
		u8 derived_key[FSCRYPT_MAX_KEY_SIZE];
		err = derive_fek(ci->ci_inode, ci, derived_key, ci->ci_mode->keysize);
		if (err)
			return err;

		if (fscrypt_sdp_use_pfk(ci)) {
			err = fscrypt_set_per_file_enc_key(ci, derived_key);
		} else if (ci->ci_policy.v2.flags &
			   FSCRYPT_POLICY_FLAG_IV_INO_LBLK_64) {
			err = setup_per_mode_enc_key(ci, mk, mk->mk_iv_ino_lblk_64_keys,
						     HKDF_CONTEXT_IV_INO_LBLK_64_KEY,
						     true);
		} else {
			err = fscrypt_set_per_file_enc_key(ci, derived_key);
		}
		memzero_explicit(derived_key, ci->ci_mode->keysize);

		if (err)
			return err;

		fscrypt_sdp_update_conv_status(ci);
		return 0;
	}
#endif

	if (ci->ci_policy.v2.flags & FSCRYPT_POLICY_FLAG_DIRECT_KEY) {
		/*
		 * DIRECT_KEY: instead of deriving per-file encryption keys, the
		 * per-file nonce will be included in all the IVs.  But unlike
		 * v1 policies, for v2 policies in this case we don't encrypt
		 * with the master key directly but rather derive a per-mode
		 * encryption key.  This ensures that the master key is
		 * consistently used only for HKDF, avoiding key reuse issues.
		 */
		err = setup_per_mode_enc_key(ci, mk, mk->mk_direct_keys,
					     HKDF_CONTEXT_DIRECT_KEY, false);
	} else if (ci->ci_policy.v2.flags &
		   FSCRYPT_POLICY_FLAG_IV_INO_LBLK_64) {
		/*
		 * IV_INO_LBLK_64: encryption keys are derived from (master_key,
		 * mode_num, filesystem_uuid), and inode number is included in
		 * the IVs.  This format is optimized for use with inline
		 * encryption hardware compliant with the UFS standard.
		 */
		err = setup_per_mode_enc_key(ci, mk, mk->mk_iv_ino_lblk_64_keys,
					     HKDF_CONTEXT_IV_INO_LBLK_64_KEY,
					     true);
	} else if (ci->ci_policy.v2.flags &
		   FSCRYPT_POLICY_FLAG_IV_INO_LBLK_32) {
		err = fscrypt_setup_iv_ino_lblk_32_key(ci, mk);
	} else {
		u8 derived_key[FSCRYPT_MAX_KEY_SIZE];

		err = fscrypt_hkdf_expand(&mk->mk_secret.hkdf,
					  HKDF_CONTEXT_PER_FILE_ENC_KEY,
					  ci->ci_nonce, FSCRYPT_FILE_NONCE_SIZE,
					  derived_key, ci->ci_mode->keysize);
		if (err)
			return err;

		err = fscrypt_set_per_file_enc_key(ci, derived_key);
		memzero_explicit(derived_key, ci->ci_mode->keysize);
	}
	if (err)
		return err;

	/* Derive a secret dirhash key for directories that need it. */
	if (need_dirhash_key) {
		err = fscrypt_derive_dirhash_key(ci, mk);
		if (err)
			return err;
	}

	return 0;
}

/*
 * Check whether the size of the given master key (@mk) is appropriate for the
 * encryption settings which a particular file will use (@ci).
 *
 * If the file uses a v1 encryption policy, then the master key must be at least
 * as long as the derived key, as this is a requirement of the v1 KDF.
 *
 * Otherwise, the KDF can accept any size key, so we enforce a slightly looser
 * requirement: we require that the size of the master key be at least the
 * maximum security strength of any algorithm whose key will be derived from it
 * (but in practice we only need to consider @ci->ci_mode, since any other
 * possible subkeys such as DIRHASH and INODE_HASH will never increase the
 * required key size over @ci->ci_mode).  This allows AES-256-XTS keys to be
 * derived from a 256-bit master key, which is cryptographically sufficient,
 * rather than requiring a 512-bit master key which is unnecessarily long.  (We
 * still allow 512-bit master keys if the user chooses to use them, though.)
 */
static bool fscrypt_valid_master_key_size(const struct fscrypt_master_key *mk,
					  const struct fscrypt_info *ci)
{
	unsigned int min_keysize;

	if (ci->ci_policy.version == FSCRYPT_POLICY_V1)
		min_keysize = ci->ci_mode->keysize;
	else
		min_keysize = ci->ci_mode->security_strength;

	if (mk->mk_secret.size < min_keysize) {
		fscrypt_warn(NULL,
			     "key with %s %*phN is too short (got %u bytes, need %u+ bytes)",
			     master_key_spec_type(&mk->mk_spec),
			     master_key_spec_len(&mk->mk_spec),
			     (u8 *)&mk->mk_spec.u,
			     mk->mk_secret.size, min_keysize);
		return false;
	}
	return true;
}

/*
 * Find the master key, then set up the inode's actual encryption key.
 *
 * If the master key is found in the filesystem-level keyring, then the
 * corresponding 'struct key' is returned in *master_key_ret with its semaphore
 * read-locked.  This is needed to ensure that only one task links the
 * fscrypt_info into ->mk_decrypted_inodes (as multiple tasks may race to create
 * an fscrypt_info for the same inode), and to synchronize the master key being
 * removed with a new inode starting to use it.
 */
static int setup_file_encryption_key(struct fscrypt_info *ci,
				     bool need_dirhash_key,
				     struct key **master_key_ret)
{
	struct key *key;
	struct fscrypt_master_key *mk = NULL;
	struct fscrypt_key_specifier mk_spec;
	int err;

	switch (ci->ci_policy.version) {
	case FSCRYPT_POLICY_V1:
		mk_spec.type = FSCRYPT_KEY_SPEC_TYPE_DESCRIPTOR;
		memcpy(mk_spec.u.descriptor,
		       ci->ci_policy.v1.master_key_descriptor,
		       FSCRYPT_KEY_DESCRIPTOR_SIZE);
		break;
	case FSCRYPT_POLICY_V2:
		mk_spec.type = FSCRYPT_KEY_SPEC_TYPE_IDENTIFIER;
		memcpy(mk_spec.u.identifier,
		       ci->ci_policy.v2.master_key_identifier,
		       FSCRYPT_KEY_IDENTIFIER_SIZE);
		break;
	default:
		WARN_ON(1);
		return -EINVAL;
	}

	key = fscrypt_find_master_key(ci->ci_inode->i_sb, &mk_spec);
	if (IS_ERR(key)) {
		if (key != ERR_PTR(-ENOKEY) ||
		    ci->ci_policy.version != FSCRYPT_POLICY_V1)
			return PTR_ERR(key);

		err = fscrypt_select_encryption_impl(ci, false);
		if (err)
			return err;

		/*
		 * As a legacy fallback for v1 policies, search for the key in
		 * the current task's subscribed keyrings too.  Don't move this
		 * to before the search of ->s_master_keys, since users
		 * shouldn't be able to override filesystem-level keys.
		 */
		return fscrypt_setup_v1_file_key_via_subscribed_keyrings(ci);
	}

	mk = key->payload.data[0];
	down_read(&key->sem);

	/* Has the secret been removed (via FS_IOC_REMOVE_ENCRYPTION_KEY)? */
	if (!is_master_key_secret_present(&mk->mk_secret)) {
		err = -ENOKEY;
		goto out_release_key;
	}

	if (!fscrypt_valid_master_key_size(mk, ci)) {
		err = -ENOKEY;
		goto out_release_key;
	}

	err = fscrypt_select_encryption_impl(ci, mk->mk_secret.is_hw_wrapped);
	if (err)
		goto out_release_key;

	switch (ci->ci_policy.version) {
	case FSCRYPT_POLICY_V1:
		err = fscrypt_setup_v1_file_key(ci, mk->mk_secret.raw);
		break;
	case FSCRYPT_POLICY_V2:
		err = fscrypt_setup_v2_file_key(ci, mk, need_dirhash_key);
		break;
	default:
		WARN_ON(1);
		err = -EINVAL;
		break;
	}
	if (err)
		goto out_release_key;

	*master_key_ret = key;
	return 0;

out_release_key:
	up_read(&key->sem);
	key_put(key);
	return err;
}

static void put_crypt_info(struct fscrypt_info *ci)
{
	struct key *key;
#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
	struct ext_fscrypt_info *ext_ci;
#endif

	if (!ci)
		return;

#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
	ext_ci = GET_EXT_CI(ci);
#ifdef CONFIG_DDAR
	dd_info_try_free(ext_ci->ci_dd_info);
#endif
#ifdef CONFIG_FSCRYPT_SDP
	fscrypt_sdp_put_sdp_info(ext_ci->ci_sdp_info);
#endif
#endif

	if (ci->ci_direct_key)
		fscrypt_put_direct_key(ci->ci_direct_key);
	else if (ci->ci_owns_key)
		fscrypt_destroy_prepared_key(&ci->ci_enc_key);

	key = ci->ci_master_key;
	if (key) {
		struct fscrypt_master_key *mk = key->payload.data[0];

		/*
		 * Remove this inode from the list of inodes that were unlocked
		 * with the master key.
		 *
		 * In addition, if we're removing the last inode from a key that
		 * already had its secret removed, invalidate the key so that it
		 * gets removed from ->s_master_keys.
		 */
		spin_lock(&mk->mk_decrypted_inodes_lock);
		list_del(&ci->ci_master_key_link);
		spin_unlock(&mk->mk_decrypted_inodes_lock);
		if (refcount_dec_and_test(&mk->mk_refcount))
			key_invalidate(key);
		key_put(key);
	}
	memzero_explicit(ci, sizeof(*ci));
#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
	kmem_cache_free(fscrypt_info_cachep, ext_ci);
#else
	kmem_cache_free(fscrypt_info_cachep, ci);
#endif
}

#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
struct fscrypt_knox_info {
	u32 flag;
	void *info; // sdp_info or dd_policy
	void *ctx; // fscrypt_sdp_context or dd_crypt_context
};
#endif

static int
fscrypt_setup_encryption_info(struct inode *inode,
			      const union fscrypt_policy *policy,
			      const u8 nonce[FSCRYPT_FILE_NONCE_SIZE],
#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
			      bool need_dirhash_key,
			      void *knox_info)
#else
			      bool need_dirhash_key)
#endif
{
#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
	struct ext_fscrypt_info *ext_crypt_info;
#endif
	struct fscrypt_info *crypt_info;
	struct fscrypt_mode *mode;
	struct key *master_key = NULL;
	int res;

	res = fscrypt_initialize(inode->i_sb->s_cop->flags);
	if (res)
		return res;

#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
	ext_crypt_info = kmem_cache_zalloc(fscrypt_info_cachep, GFP_KERNEL);
	if (!ext_crypt_info)
		return -ENOMEM;
	crypt_info = &ext_crypt_info->fscrypt_info;
#ifdef CONFIG_DDAR
	ext_crypt_info->ci_dd_info = NULL;
#endif
#ifdef CONFIG_FSCRYPT_SDP
	ext_crypt_info->ci_sdp_info = NULL;
#endif

#else
	crypt_info = kmem_cache_zalloc(fscrypt_info_cachep, GFP_KERNEL);
	if (!crypt_info)
		return -ENOMEM;
#endif

	crypt_info->ci_inode = inode;
	crypt_info->ci_policy = *policy;
	memcpy(crypt_info->ci_nonce, nonce, FSCRYPT_FILE_NONCE_SIZE);

	mode = select_encryption_mode(&crypt_info->ci_policy, inode);
	if (IS_ERR(mode)) {
		res = PTR_ERR(mode);
		goto out;
	}
	WARN_ON(mode->ivsize > FSCRYPT_MAX_IV_SIZE);
	crypt_info->ci_mode = mode;

#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
	if (knox_info) {
		struct fscrypt_knox_info *knoxInfo = (struct fscrypt_knox_info *) knox_info;
#ifdef CONFIG_FSCRYPT_SDP
		if (fscrypt_sdp_protected(knoxInfo->flag)) {
			ext_crypt_info->ci_sdp_info = fscrypt_sdp_alloc_sdp_info();
			if (!ext_crypt_info->ci_sdp_info) {
				res = -ENOMEM;
				goto out;
			}

			if (knoxInfo->ctx) {
				struct fscrypt_sdp_context *sdp_ctx = (struct fscrypt_sdp_context *) knoxInfo->ctx;
				ext_crypt_info->ci_sdp_info->sdp_flags =
						FSCRYPT_SDP_PARSE_FLAG_SDP_ONLY(knoxInfo->flag);
#ifdef CONFIG_SDP_KEY_DUMP
				ext_crypt_info->ci_sdp_info->sdp_flags |=
						FSCRYPT_SDP_PARSE_FLAG_SDP_TRACE_ONLY(knoxInfo->flag);
#endif

				ext_crypt_info->ci_sdp_info->engine_id = sdp_ctx->engine_id;
				ext_crypt_info->ci_sdp_info->sdp_dek.type = sdp_ctx->sdp_dek_type;
				ext_crypt_info->ci_sdp_info->sdp_dek.len = sdp_ctx->sdp_dek_len;
				DEK_LOGD("sdp_flags = 0x%08x, engine_id = %d\n", ext_crypt_info->ci_sdp_info->sdp_flags, sdp_ctx->engine_id);
				memcpy(ext_crypt_info->ci_sdp_info->sdp_dek.buf, sdp_ctx->sdp_dek_buf,
						sizeof(ext_crypt_info->ci_sdp_info->sdp_dek.buf));
				memcpy(ext_crypt_info->ci_sdp_info->sdp_en_buf, sdp_ctx->sdp_en_buf,
						sizeof(ext_crypt_info->ci_sdp_info->sdp_en_buf));

				if (S_ISDIR(inode->i_mode))
					ext_crypt_info->ci_sdp_info->sdp_flags |= SDP_IS_DIRECTORY;

			} else {
				// Called from fscrypt_prepare_new_inode()
				struct sdp_info *sdpInfo = (struct sdp_info *) knoxInfo->info;
				ext_crypt_info->ci_sdp_info->sdp_flags =
						FSCRYPT_SDP_PARSE_FLAG_SDP_ONLY(knoxInfo->flag);
#ifdef CONFIG_SDP_KEY_DUMP
				ext_crypt_info->ci_sdp_info->sdp_flags |=
						FSCRYPT_SDP_PARSE_FLAG_SDP_TRACE_ONLY(knoxInfo->flag);
#endif

				ext_crypt_info->ci_sdp_info->engine_id = sdpInfo->engine_id;
				ext_crypt_info->ci_sdp_info->sdp_dek.type = sdpInfo->sdp_dek.type;
				ext_crypt_info->ci_sdp_info->sdp_dek.len = sdpInfo->sdp_dek.len;
				memcpy(ext_crypt_info->ci_sdp_info->sdp_dek.buf, sdpInfo->sdp_dek.buf,
						sizeof(ext_crypt_info->ci_sdp_info->sdp_dek.buf));
				memcpy(ext_crypt_info->ci_sdp_info->sdp_en_buf, sdpInfo->sdp_en_buf,
						sizeof(ext_crypt_info->ci_sdp_info->sdp_en_buf));

				if (S_ISDIR(inode->i_mode))
					ext_crypt_info->ci_sdp_info->sdp_flags |= SDP_IS_DIRECTORY;
			}
		}
#endif
	}
#endif

	res = setup_file_encryption_key(crypt_info, need_dirhash_key,
					&master_key);
	if (res)
		goto out;

#ifdef CONFIG_DDAR
	if (knox_info) {
		struct fscrypt_knox_info *knoxInfo = (struct fscrypt_knox_info *) knox_info;
		if (fscrypt_ddar_protected(knoxInfo->flag)) {
			struct dd_info *di = alloc_dd_info(inode, knoxInfo->info, knoxInfo->ctx);
			if (IS_ERR(di)) {
				dd_error("%s - failed to alloc dd_info(%d)\n", __func__, __LINE__);
				res = PTR_ERR(di);

				goto out;
			}

			ext_crypt_info->ci_dd_info = di;
		}
	}
#endif

	/*
	 * For existing inodes, multiple tasks may race to set ->i_crypt_info.
	 * So use cmpxchg_release().  This pairs with the smp_load_acquire() in
	 * fscrypt_get_info().  I.e., here we publish ->i_crypt_info with a
	 * RELEASE barrier so that other tasks can ACQUIRE it.
	 */
	if (cmpxchg_release(&inode->i_crypt_info, NULL, crypt_info) == NULL) {
		/*
		 * We won the race and set ->i_crypt_info to our crypt_info.
		 * Now link it into the master key's inode list.
		 */
		if (master_key) {
			struct fscrypt_master_key *mk =
				master_key->payload.data[0];

			refcount_inc(&mk->mk_refcount);
			crypt_info->ci_master_key = key_get(master_key);
			spin_lock(&mk->mk_decrypted_inodes_lock);
			list_add(&crypt_info->ci_master_key_link,
				 &mk->mk_decrypted_inodes);
			spin_unlock(&mk->mk_decrypted_inodes_lock);
		}
		crypt_info = NULL;
	}
#ifdef CONFIG_DDAR
	if (crypt_info == NULL) {
		if (ext_crypt_info->ci_dd_info) {
			fscrypt_dd_inc_count();
		}
	}
#endif
#ifdef CONFIG_FSCRYPT_SDP
	if (crypt_info == NULL) //Call only when i_crypt_info is loaded initially
		fscrypt_sdp_finalize_tasks(inode);
#endif
	res = 0;
out:
	if (master_key) {
		up_read(&master_key->sem);
		key_put(master_key);
	}
	put_crypt_info(crypt_info);
	return res;
}

/**
 * fscrypt_get_encryption_info() - set up an inode's encryption key
 * @inode: the inode to set up the key for.  Must be encrypted.
 * @allow_unsupported: if %true, treat an unsupported encryption policy (or
 *		       unrecognized encryption context) the same way as the key
 *		       being unavailable, instead of returning an error.  Use
 *		       %false unless the operation being performed is needed in
 *		       order for files (or directories) to be deleted.
 *
 * Set up ->i_crypt_info, if it hasn't already been done.
 *
 * Note: unless ->i_crypt_info is already set, this isn't %GFP_NOFS-safe.  So
 * generally this shouldn't be called from within a filesystem transaction.
 *
 * Return: 0 if ->i_crypt_info was set or was already set, *or* if the
 *	   encryption key is unavailable.  (Use fscrypt_has_encryption_key() to
 *	   distinguish these cases.)  Also can return another -errno code.
 */
int fscrypt_get_encryption_info(struct inode *inode, bool allow_unsupported)
{
	int res;
	union fscrypt_context ctx;
	union fscrypt_policy policy;

	if (fscrypt_has_encryption_key(inode)) {
#ifdef CONFIG_DDAR
		if (fscrypt_dd_encrypted_inode(inode) && fscrypt_dd_is_locked()) {
			dd_error("Failed to open a DDAR-protected file in lock state (ino:%ld)\n", inode->i_ino);
			return -ENOKEY;
		}
#endif
		return 0;
	}

	res = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx));
	if (res < 0) {
		if (res == -ERANGE && allow_unsupported)
			return 0;
		fscrypt_warn(inode, "Error %d getting encryption context", res);
		return res;
	}

#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
	switch (ctx.version) {
	case FSCRYPT_CONTEXT_V1: {
		if (res == offsetof(struct fscrypt_context_v1, knox_flags)) {
			ctx.v1.knox_flags = 0;
			res = sizeof(ctx.v1);
		}
		break;
	}
	case FSCRYPT_CONTEXT_V2: {
		if (res == offsetof(struct fscrypt_context_v2, knox_flags)) {
			ctx.v2.knox_flags = 0;
			res = sizeof(ctx.v2);
		}
		break;
	}
	}
#endif

	res = fscrypt_policy_from_context(&policy, &ctx, res);
	if (res) {
		if (allow_unsupported)
			return 0;
		fscrypt_warn(inode,
			     "Unrecognized or corrupt encryption context");
		return res;
	}

	if (!fscrypt_supported_policy(&policy, inode)) {
		if (allow_unsupported)
			return 0;
		return -EINVAL;
	}

#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
	if (fscrypt_has_knox_flags(&ctx)) {
		struct fscrypt_knox_info knox_info;
#ifdef CONFIG_DDAR
		struct dd_crypt_context crypt_context;
#endif
#ifdef CONFIG_FSCRYPT_SDP
		struct fscrypt_sdp_context sdp_ctx;
#endif

		memset(&knox_info, 0, sizeof(knox_info));
		knox_info.flag = fscrypt_knox_flags_from_context(&ctx);
#ifdef CONFIG_DDAR
		if (fscrypt_ddar_protected(knox_info.flag)) {
			if (dd_read_crypt_context(inode, &crypt_context) != sizeof(struct dd_crypt_context)) {
				dd_error("%s: failed to read dd crypt context (ino:%ld)\n", __func__, inode->i_ino);
				return -EINVAL;
			}

			knox_info.info = (void *) &crypt_context.policy;
			knox_info.ctx = (void *) &crypt_context;
		}
#endif
#ifdef CONFIG_FSCRYPT_SDP
		if (fscrypt_sdp_protected(knox_info.flag)) {
			if (fscrypt_sdp_get_context(inode, &sdp_ctx, sizeof(sdp_ctx)) != sizeof(sdp_ctx)) {
				printk("%s: failed to get sdp context (ino:%ld)\n", __func__, inode->i_ino);
				return -EINVAL;
			}

			knox_info.ctx = (void *) &sdp_ctx;
		}
#endif
		res = fscrypt_setup_encryption_info(inode, &policy,
						    fscrypt_context_nonce(&ctx),
						    IS_CASEFOLDED(inode) &&
						    S_ISDIR(inode->i_mode), &knox_info);
	} else {
		res = fscrypt_setup_encryption_info(inode, &policy,
						    fscrypt_context_nonce(&ctx),
						    IS_CASEFOLDED(inode) &&
						    S_ISDIR(inode->i_mode), NULL);
	}
#else
	res = fscrypt_setup_encryption_info(inode, &policy,
					    fscrypt_context_nonce(&ctx),
					    IS_CASEFOLDED(inode) &&
					    S_ISDIR(inode->i_mode));
#endif

	if (res == -ENOPKG && allow_unsupported) /* Algorithm unavailable? */
		res = 0;
	if (res == -ENOKEY)
		res = 0;
	return res;
}

/**
 * fscrypt_prepare_new_inode() - prepare to create a new inode in a directory
 * @dir: a possibly-encrypted directory
 * @inode: the new inode.  ->i_mode must be set already.
 *	   ->i_ino doesn't need to be set yet.
 * @encrypt_ret: (output) set to %true if the new inode will be encrypted
 *
 * If the directory is encrypted, set up its ->i_crypt_info in preparation for
 * encrypting the name of the new file.  Also, if the new inode will be
 * encrypted, set up its ->i_crypt_info and set *encrypt_ret=true.
 *
 * This isn't %GFP_NOFS-safe, and therefore it should be called before starting
 * any filesystem transaction to create the inode.  For this reason, ->i_ino
 * isn't required to be set yet, as the filesystem may not have set it yet.
 *
 * This doesn't persist the new inode's encryption context.  That still needs to
 * be done later by calling fscrypt_set_context().
 *
 * Return: 0 on success, -ENOKEY if the encryption key is missing, or another
 *	   -errno code
 */
int fscrypt_prepare_new_inode(struct inode *dir, struct inode *inode,
			      bool *encrypt_ret)
{
	const union fscrypt_policy *policy;
#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
	struct fscrypt_info *ci;
	struct ext_fscrypt_info *ext_ci;
#endif
	u8 nonce[FSCRYPT_FILE_NONCE_SIZE];

	policy = fscrypt_policy_to_inherit(dir);
	if (policy == NULL)
		return 0;
	if (IS_ERR(policy))
		return PTR_ERR(policy);

	if (WARN_ON_ONCE(inode->i_mode == 0))
		return -EINVAL;

	/*
	 * Only regular files, directories, and symlinks are encrypted.
	 * Special files like device nodes and named pipes aren't.
	 */
	if (!S_ISREG(inode->i_mode) &&
	    !S_ISDIR(inode->i_mode) &&
	    !S_ISLNK(inode->i_mode))
		return 0;

	*encrypt_ret = true;

	get_random_bytes(nonce, FSCRYPT_FILE_NONCE_SIZE);

#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
	ci = fscrypt_has_dar_info(dir);
	if (ci) {
		struct fscrypt_knox_info knox_info;
#ifdef CONFIG_FSCRYPT_SDP
		struct sdp_info sdpInfo;
#endif

		memset(&knox_info, 0, sizeof(knox_info));

		ext_ci = GET_EXT_CI(ci);
#ifdef CONFIG_DDAR
		if (ext_ci->ci_dd_info) {
			if (fscrypt_dd_is_locked()
					&& !fscrypt_dd_flg_gid_restricted(
							(ext_ci->ci_dd_info->policy.flags << FSCRYPT_KNOX_FLG_DDAR_SHIFT), 0)) {
				dd_error("%s - Failed to open a DDAR-protected (CE) file in lock state (ino:%ld)\n", __func__, inode->i_ino);
				return -ENOKEY;
			}

			knox_info.flag |= ((ext_ci->ci_dd_info->policy.flags << FSCRYPT_KNOX_FLG_DDAR_SHIFT) & FSCRYPT_KNOX_FLG_DDAR_MASK);
			knox_info.info = (void *) &ext_ci->ci_dd_info->policy;
			knox_info.ctx = NULL;
		}
#endif
#ifdef CONFIG_FSCRYPT_SDP
		if (ext_ci->ci_sdp_info) {
			int res;
			memset(&sdpInfo, 0, sizeof(sdpInfo));
			res = fscrypt_sdp_inherit_info(dir, inode, &knox_info.flag, &sdpInfo);
			if (res) {
				printk_once(KERN_WARNING
						"%s: Failed to set sensitive ongoing flag (err:%d)\n", __func__, res);
				return res;
			}
			knox_info.info = (void *) &sdpInfo;
			knox_info.ctx = NULL;
		}
#endif
		return fscrypt_setup_encryption_info(inode, policy, nonce,
						     IS_CASEFOLDED(dir) &&
						     S_ISDIR(inode->i_mode), &knox_info);
	} else {
		return fscrypt_setup_encryption_info(inode, policy, nonce,
						     IS_CASEFOLDED(dir) &&
						     S_ISDIR(inode->i_mode), NULL);
	}
#else
	return fscrypt_setup_encryption_info(inode, policy, nonce,
					     IS_CASEFOLDED(dir) &&
					     S_ISDIR(inode->i_mode));
#endif
}
EXPORT_SYMBOL_GPL(fscrypt_prepare_new_inode);

/**
 * fscrypt_put_encryption_info() - free most of an inode's fscrypt data
 * @inode: an inode being evicted
 *
 * Free the inode's fscrypt_info.  Filesystems must call this when the inode is
 * being evicted.  An RCU grace period need not have elapsed yet.
 */
void fscrypt_put_encryption_info(struct inode *inode)
{
#ifdef CONFIG_DDAR
	if (inode->i_crypt_info) {
		struct ext_fscrypt_info *ext_ci = GET_EXT_CI(inode->i_crypt_info);
		if (ext_ci->ci_dd_info) {
			fscrypt_dd_dec_count();
		}
	}
#endif

#ifdef CONFIG_FSCRYPT_SDP
	fscrypt_sdp_cache_remove_inode_num(inode);
#endif
	put_crypt_info(inode->i_crypt_info);
	inode->i_crypt_info = NULL;
}
EXPORT_SYMBOL(fscrypt_put_encryption_info);

/**
 * fscrypt_free_inode() - free an inode's fscrypt data requiring RCU delay
 * @inode: an inode being freed
 *
 * Free the inode's cached decrypted symlink target, if any.  Filesystems must
 * call this after an RCU grace period, just before they free the inode.
 */
void fscrypt_free_inode(struct inode *inode)
{
	if (IS_ENCRYPTED(inode) && S_ISLNK(inode->i_mode)) {
		kfree(inode->i_link);
		inode->i_link = NULL;
	}
}
EXPORT_SYMBOL(fscrypt_free_inode);

/**
 * fscrypt_drop_inode() - check whether the inode's master key has been removed
 * @inode: an inode being considered for eviction
 *
 * Filesystems supporting fscrypt must call this from their ->drop_inode()
 * method so that encrypted inodes are evicted as soon as they're no longer in
 * use and their master key has been removed.
 *
 * Return: 1 if fscrypt wants the inode to be evicted now, otherwise 0
 */
int fscrypt_drop_inode(struct inode *inode)
{
	const struct fscrypt_info *ci = fscrypt_get_info(inode);
	const struct fscrypt_master_key *mk;

	/*
	 * If ci is NULL, then the inode doesn't have an encryption key set up
	 * so it's irrelevant.  If ci_master_key is NULL, then the master key
	 * was provided via the legacy mechanism of the process-subscribed
	 * keyrings, so we don't know whether it's been removed or not.
	 */
	if (!ci || !ci->ci_master_key)
		return 0;
	mk = ci->ci_master_key->payload.data[0];

	/*
	 * With proper, non-racy use of FS_IOC_REMOVE_ENCRYPTION_KEY, all inodes
	 * protected by the key were cleaned by sync_filesystem().  But if
	 * userspace is still using the files, inodes can be dirtied between
	 * then and now.  We mustn't lose any writes, so skip dirty inodes here.
	 */
	if (inode->i_state & I_DIRTY_ALL)
		return 0;

	/*
	 * Note: since we aren't holding the key semaphore, the result here can
	 * immediately become outdated.  But there's no correctness problem with
	 * unnecessarily evicting.  Nor is there a correctness problem with not
	 * evicting while iput() is racing with the key being removed, since
	 * then the thread removing the key will either evict the inode itself
	 * or will correctly detect that it wasn't evicted due to the race.
	 */
	return !is_master_key_secret_present(&mk->mk_secret);
}
EXPORT_SYMBOL_GPL(fscrypt_drop_inode);

#ifdef CONFIG_FSCRYPT_SDP
static inline int __find_and_derive_mode_key(
		struct fscrypt_key *fskey,
		struct fscrypt_info *ci,
		struct fscrypt_master_key *mk,
		u8 hkdf_context, bool include_fs_uuid)
{
	const struct inode *inode = ci->ci_inode;
	/*const */struct super_block *sb = inode->i_sb;
	struct fscrypt_mode *mode = ci->ci_mode;
	const u8 mode_num = mode - fscrypt_modes;
	u8 mode_key[FSCRYPT_MAX_KEY_SIZE];
	u8 hkdf_info[sizeof(mode_num) + sizeof(sb->s_uuid)];
	unsigned int hkdf_infolen = 0;
	int err;

	if (WARN_ON(mode_num > FSCRYPT_MODE_MAX))
		return -EINVAL;

	mutex_lock(&fscrypt_mode_key_setup_mutex);

	if (mk->mk_secret.is_hw_wrapped && S_ISREG(inode->i_mode)
			&& !fscrypt_sdp_use_hkdf_expanded_key(ci)) {
		u8 kdf_key[32];
		unsigned int kdf_key_size = 32;
		if (!fscrypt_using_inline_encryption(ci)) {
			fscrypt_warn(ci->ci_inode,
				     "Hardware-wrapped keys require inline encryption (-o inlinecrypt)");
			err = -EINVAL;
			goto out_unlock;
		}
//		err = -EOPNOTSUPP;
		err = fscrypt_derive_raw_secret(sb, mk->mk_secret.raw,
						mk->mk_secret.size,
						kdf_key, kdf_key_size);
		if (err)
			goto out_unlock;

		memcpy(fskey->raw, kdf_key, kdf_key_size);
		fskey->size = kdf_key_size;
		memzero_explicit(kdf_key, kdf_key_size);

	} else {
		BUILD_BUG_ON(sizeof(mode_num) != 1);
		BUILD_BUG_ON(sizeof(sb->s_uuid) != 16);
		BUILD_BUG_ON(sizeof(hkdf_info) != 17);
		hkdf_info[hkdf_infolen++] = mode_num;
		if (include_fs_uuid) {
			memcpy(&hkdf_info[hkdf_infolen], &sb->s_uuid,
				   sizeof(sb->s_uuid));
			hkdf_infolen += sizeof(sb->s_uuid);
		}
		err = fscrypt_hkdf_expand(&mk->mk_secret.hkdf,
					  hkdf_context, hkdf_info, hkdf_infolen,
					  mode_key, mode->keysize);
		if (err)
			goto out_unlock;

		memcpy(fskey->raw, mode_key, mode->keysize);
		fskey->size = mode->keysize;
		memzero_explicit(mode_key, mode->keysize);
	}

out_unlock:
	mutex_unlock(&fscrypt_mode_key_setup_mutex);
	return err;
}

static inline int __find_and_derive_fskey(
					struct fscrypt_info *ci,
					struct fscrypt_key *fskey)
{
	struct key *key;
	struct fscrypt_master_key *mk = NULL;
	struct fscrypt_key_specifier mk_spec;
	int err;

	switch (ci->ci_policy.version) {
//	case FSCRYPT_POLICY_V1:
//		mk_spec.type = FSCRYPT_KEY_SPEC_TYPE_DESCRIPTOR;
//		memcpy(mk_spec.u.descriptor,
//		       ci->ci_policy.v1.master_key_descriptor,
//		       FSCRYPT_KEY_DESCRIPTOR_SIZE);
//		break;
	case FSCRYPT_POLICY_V2:
		mk_spec.type = FSCRYPT_KEY_SPEC_TYPE_IDENTIFIER;
		memcpy(mk_spec.u.identifier,
		       ci->ci_policy.v2.master_key_identifier,
		       FSCRYPT_KEY_IDENTIFIER_SIZE);
		break;
	default:
		WARN_ON(1);
		return -EINVAL;
	}

	key = fscrypt_find_master_key(ci->ci_inode->i_sb, &mk_spec);
	if (IS_ERR(key))
		return PTR_ERR(key);

	mk = key->payload.data[0];
	down_read(&key->sem);

	/* Has the secret been removed (via FS_IOC_REMOVE_ENCRYPTION_KEY)? */
	if (!is_master_key_secret_present(&mk->mk_secret)) {
		err = -ENOKEY;
		goto out_release_key;
	}

	/*
	 * Require that the master key be at least as long as the derived key.
	 * Otherwise, the derived key cannot possibly contain as much entropy as
	 * that required by the encryption mode it will be used for.  For v1
	 * policies it's also required for the KDF to work at all.
	 */
	if (mk->mk_secret.size < ci->ci_mode->keysize) {
		fscrypt_warn(NULL,
			     "key with %s %*phN is too short (got %u bytes, need %u+ bytes)",
			     master_key_spec_type(&mk_spec),
			     master_key_spec_len(&mk_spec), (u8 *)&mk_spec.u,
			     mk->mk_secret.size, ci->ci_mode->keysize);
		err = -ENOKEY;
		goto out_release_key;
	}

	err = fscrypt_select_encryption_impl(ci, mk->mk_secret.is_hw_wrapped);
	if (err)
		goto out_release_key;

	if (mk->mk_secret.is_hw_wrapped &&
	    !(ci->ci_policy.v2.flags & (FSCRYPT_POLICY_FLAG_IV_INO_LBLK_64 |
					FSCRYPT_POLICY_FLAG_IV_INO_LBLK_32))) {
		fscrypt_warn(ci->ci_inode,
			     "Hardware-wrapped keys are only supported with IV_INO_LBLK policies");
		err = -EINVAL;
		goto out_release_key;
	}

	if (ci->ci_policy.v2.flags & FSCRYPT_POLICY_FLAG_DIRECT_KEY) {
		err = __find_and_derive_mode_key(fskey, ci, mk,
				HKDF_CONTEXT_DIRECT_KEY, false);
	} else if (ci->ci_policy.v2.flags &
		   FSCRYPT_POLICY_FLAG_IV_INO_LBLK_64) {
		err = __find_and_derive_mode_key(fskey, ci, mk,
				HKDF_CONTEXT_IV_INO_LBLK_64_KEY, true);
		if (err == 0)
			fskey->size = ci->ci_mode->keysize;
	} else if (ci->ci_policy.v2.flags &
		   FSCRYPT_POLICY_FLAG_IV_INO_LBLK_32) {
		err = -EOPNOTSUPP;
	} else {
		err = fscrypt_hkdf_expand(&mk->mk_secret.hkdf,
					  HKDF_CONTEXT_PER_FILE_ENC_KEY,
					  ci->ci_nonce,
					  FSCRYPT_FILE_NONCE_SIZE,
					  fskey->raw, ci->ci_mode->keysize);
		if (err == 0)
			fskey->size = ci->ci_mode->keysize;
	}

out_release_key:
	up_read(&key->sem);
	key_put(key);
	return err;
}

/* The function is only for regular files */
static int derive_fek(struct inode *inode,
						struct fscrypt_info *crypt_info,
						u8 *fek, u32 fek_len)
{
	int res = 0;
	/*
	 * 1. [ Native / Uninitialized / To_sensitive ]  --> Plain fek
	 * 2. [ Native / Uninitialized / Non_sensitive ] --> Plain fek
	 */
	if (fscrypt_sdp_is_uninitialized(crypt_info))
		res = fscrypt_sdp_derive_uninitialized_dek(crypt_info, fek, fek_len);
	/*
	 * 3. [ Native / Initialized / Sensitive ]     --> { fek }_SDPK
	 * 4. [ Non_native / Initialized / Sensitive ] --> { fek }_SDPK
	 */
	else if (fscrypt_sdp_is_sensitive(crypt_info))
		res = fscrypt_sdp_derive_dek(crypt_info, fek, fek_len);
	/*
	 * 5. [ Native / Initialized / Non_sensitive ] --> { fek }_cekey
	 */
	else if (fscrypt_sdp_is_native(crypt_info))
		res = fscrypt_sdp_derive_fek(inode, crypt_info, fek, fek_len);
	/*
	 * else { N/A }
	 *
	 * Not classified file.
	 * 6. [ Non_native / Initialized / Non_sensitive ]
	 * 7. [ Non_native / Initialized / To_sensitive ]
	 */
	return res;
}

int fscrypt_get_encryption_key(
		struct fscrypt_info *crypt_info,
		struct fscrypt_key *key)
{
	struct fscrypt_key *kek = NULL;
	int res;

	if (!crypt_info || !(kek = key))
		return -EINVAL;

	res = fscrypt_get_encryption_kek(crypt_info, kek);

	return res;
}
EXPORT_SYMBOL(fscrypt_get_encryption_key);

int fscrypt_get_encryption_key_classified(
		struct fscrypt_info *crypt_info,
		struct fscrypt_key *key)
{
	u8 *derived_key;
	int err;

	if (!crypt_info)
		return -EINVAL;

	derived_key = kmalloc(crypt_info->ci_mode->keysize, GFP_NOFS);
	if (!derived_key)
		return -ENOMEM;

	err = derive_fek(crypt_info->ci_inode, crypt_info, derived_key, crypt_info->ci_mode->keysize);
	if (err)
		goto out;

	memcpy(key->raw, derived_key, crypt_info->ci_mode->keysize);
	key->size = crypt_info->ci_mode->keysize;

out:
	kfree_sensitive(derived_key);
	return err;
}
EXPORT_SYMBOL(fscrypt_get_encryption_key_classified);

int fscrypt_get_encryption_kek(
		struct fscrypt_info *crypt_info,
		struct fscrypt_key *kek)
{
	int res;

	if (!crypt_info)
		return -EINVAL;

	res = __find_and_derive_fskey(crypt_info, kek);
	return res;
}
EXPORT_SYMBOL(fscrypt_get_encryption_kek);
#endif

#if defined(CONFIG_FSCRYPT_SDP) || defined(CONFIG_DDAR)
int fscrypt_set_knox_dar_context(struct inode *inode, struct fscrypt_info *ci,
		union fscrypt_context *ctx, int ctxsize, void *fs_data)
{
	if (fscrypt_has_dar_info(inode)) {
		int res = 0;
		struct ext_fscrypt_info *ext_ci = GET_EXT_CI(ci);

#ifdef CONFIG_DDAR
		if (ext_ci->ci_dd_info) {
			res = fscrypt_set_knox_ddar_flags(ctx, ci);
			if (res) {
				dd_error("failed to set knox ddar flag\n");
				return res;
			}

			switch (ctx->version) {
			case FSCRYPT_CONTEXT_V1: {
				if (ctx->v1.knox_flags != 0)
					ctxsize = sizeof(ctx->v1);
				break;
			}
			case FSCRYPT_CONTEXT_V2: {
				if (ctx->v2.knox_flags != 0)
					ctxsize = sizeof(ctx->v2);
				break;
			}
			}

			res = inode->i_sb->s_cop->set_context(inode, ctx, ctxsize, fs_data);
			if (res) {
				dd_error("failed to set context (%ld)\n", inode->i_ino);
				return res;
			}

			res = dd_write_crypt_context(inode, &ext_ci->ci_dd_info->crypt_context, fs_data);
			dd_verbose("%s - ino : %ld, policy.flag:%x, res : %d",
					__func__, inode->i_ino, ext_ci->ci_dd_info->policy.flags, res);
		}
#endif
#ifdef CONFIG_FSCRYPT_SDP
		if (ext_ci->ci_sdp_info) {
			struct fscrypt_sdp_context sdp_ctx;

			res = fscrypt_set_knox_sdp_flags(ctx, ci);
			if (res) {
				printk_once(KERN_WARNING
						"%s: Failed to set sensitive ongoing flag (err:%d)\n", __func__, res);
				return res;
			}

			switch (ctx->version) {
			case FSCRYPT_CONTEXT_V1: {
				if (ctx->v1.knox_flags != 0)
					ctxsize = sizeof(ctx->v1);
				break;
			}
			case FSCRYPT_CONTEXT_V2: {
				if (ctx->v2.knox_flags != 0)
					ctxsize = sizeof(ctx->v2);
				break;
			}
			}

			res = inode->i_sb->s_cop->set_context(inode, ctx, ctxsize, fs_data);
			if (res) {
				printk("failed to set context (%ld)\n", inode->i_ino);
				return res;
			}

			sdp_ctx.engine_id = ext_ci->ci_sdp_info->engine_id;
			sdp_ctx.sdp_dek_type = ext_ci->ci_sdp_info->sdp_dek.type;
			sdp_ctx.sdp_dek_len = ext_ci->ci_sdp_info->sdp_dek.len;
			memcpy(sdp_ctx.sdp_dek_buf, ext_ci->ci_sdp_info->sdp_dek.buf, DEK_MAXLEN);
	//		memset(sdp_ctx.sdp_en_buf, 0, MAX_EN_BUF_LEN); // Keep it as dummy
			memcpy(sdp_ctx.sdp_en_buf, ext_ci->ci_sdp_info->sdp_en_buf, MAX_EN_BUF_LEN);

			/* Update SDP Context */
			res = fscrypt_sdp_set_context_nolock(inode, &sdp_ctx, sizeof(sdp_ctx), fs_data);
			if (res)
				printk("fscrypt_set_context: failed to set sdp context (err:%d)\n", res);
		}
#endif
		return res;
	}
	return -EAGAIN;
}
#endif
