#include <v1/exynos-mif-profiler.h>

/************************************************************************
 *				HELPER					*
 ************************************************************************/

static inline void calc_delta(u64 *result_table, u64 *prev_table, u64 *cur_table, int size)
{
	int i;
	u64 delta, cur;

	for (i = 0; i < size; i++) {
		cur = cur_table[i];
		delta = cur - prev_table[i];
		result_table[i] = delta;
		prev_table[i] = cur;
	}
}

/************************************************************************
 *				SUPPORT-PROFILER				*
 ************************************************************************/
u32 mifpro_get_table_cnt(s32 id)
{
	return profiler.table_cnt;
}

u32 mifpro_get_freq_table(s32 id, u32 *table)
{
	int idx;
	for (idx = 0; idx < profiler.table_cnt; idx++)
		table[idx] = profiler.table[idx].freq;

	return idx;
}

u32 mifpro_get_max_freq(s32 id)
{
	return exynos_pm_qos_request(profiler.freq_infos.pm_qos_class_max);
}

u32 mifpro_get_min_freq(s32 id)
{
	return exynos_pm_qos_request(profiler.freq_infos.pm_qos_class);
}

u32 mifpro_get_freq(s32 id)
{
	return profiler.result[PROFILER].fc_result.freq[CS_ACTIVE];
}

void mifpro_get_power(s32 id, u64 *dyn_power, u64 *st_power)
{
	*dyn_power = profiler.result[PROFILER].fc_result.dyn_power;
	*st_power = profiler.result[PROFILER].fc_result.st_power;
}

void mifpro_get_power_change(s32 id, s32 freq_delta_ratio,
			u32 *freq, u64 *dyn_power, u64 *st_power)
{
	struct mif_profile_result *result = &profiler.result[PROFILER];
	struct freq_cstate_result *fc_result = &result->fc_result;
	int flag = (STATE_SCALE_WO_SPARE | STATE_SCALE_CNT);
	u64 dyn_power_backup;

	get_power_change(profiler.table, profiler.table_cnt,
		profiler.cur_freq_idx, profiler.min_freq_idx, profiler.max_freq_idx,
		result->freq_stats[0], fc_result->time[CLK_OFF], freq_delta_ratio,
		fc_result->profile_time, result->avg_temp, flag, dyn_power, st_power, freq);

	dyn_power_backup = *dyn_power;

	get_power_change(profiler.table, profiler.table_cnt,
		profiler.cur_freq_idx, profiler.min_freq_idx, profiler.max_freq_idx,
		fc_result->time[CS_ACTIVE], fc_result->time[CLK_OFF], freq_delta_ratio,
		fc_result->profile_time, result->avg_temp, flag, dyn_power, st_power, freq);

	*dyn_power = dyn_power_backup;
}

u32 mifpro_get_active_pct(s32 id)
{
	return profiler.result[PROFILER].fc_result.ratio[CS_ACTIVE];
}

s32 mifpro_get_temp(s32 id)
{
	return profiler.result[PROFILER].avg_temp;
}
void mifpro_set_margin(s32 id, s32 margin)
{
	return;
}

u32 mifpro_update_profile(int user);
u32 mifpro_update_mode(s32 id, int mode)
{
	int i;

	if (profiler.enabled == 0 && mode == 1) {
		struct freq_cstate *fc = &profiler.fc;
		struct freq_cstate_snapshot *fc_snap = &profiler.fc_snap[PROFILER];

		sync_fcsnap_with_cur(fc, fc_snap, profiler.table_cnt);
#if defined(CONFIG_SOC_S5E9925_EVT0) || defined(CONFIG_SOC_S5E8825)
		exynos_bcm_calc_enable(1);
#endif

		profiler.enabled = mode;

		exynos_devfreq_set_profile(profiler.devfreq_type, 1);
	}
	else if (profiler.enabled == 1 && mode == 0) {
		exynos_devfreq_set_profile(profiler.devfreq_type, 0);
#if defined(CONFIG_SOC_S5E9925_EVT0) || defined(CONFIG_SOC_S5E8825)
		exynos_bcm_calc_enable(0);
#endif
		profiler.enabled = mode;

		// clear
		for (i = 0; i < NUM_OF_CSTATE; i++) {
			memset(profiler.result[PROFILER].fc_result.time[i], 0, sizeof(ktime_t) * profiler.table_cnt);
			profiler.result[PROFILER].fc_result.ratio[i] = 0;
			profiler.result[PROFILER].fc_result.freq[i] = 0;
			memset(profiler.fc.time[i], 0, sizeof(ktime_t) * profiler.table_cnt);
			memset(profiler.fc_snap[PROFILER].time[i], 0, sizeof(ktime_t) * profiler.table_cnt);
		}
		profiler.result[PROFILER].fc_result.dyn_power = 0;
		profiler.result[PROFILER].fc_result.st_power = 0;
		profiler.result[PROFILER].fc_result.profile_time = 0;
		profiler.result[PROFILER].freq_stats0_sum = 0;
		profiler.result[PROFILER].freq_stats0_avg = 0;
		profiler.result[PROFILER].freq_stats_ratio = 0;
		profiler.result[PROFILER].freq_stats1_sum = 0;
		profiler.fc_snap[PROFILER].last_snap_time = 0;
#if defined(CONFIG_SOC_S5E9925_EVT0) || defined(CONFIG_SOC_S5E8825)
		for (i = 0; i < 4; i++) {
			memset(profiler.freq_stats[i], 0, sizeof(u64) * profiler.table_cnt);
			memset(profiler.freq_stats_snap[i], 0, sizeof(u64) * profiler.table_cnt);
			memset(profiler.result[PROFILER].freq_stats[i], 0, sizeof(u64) * profiler.table_cnt);

		}

#else
		memset(profiler.prev_wow_profile, 0, sizeof(struct exynos_wow_profile) * profiler.table_cnt);
#endif

		return 0;
	}

	mifpro_update_profile(PROFILER);

	return 0;
}

u64 mifpro_get_freq_stats0_sum(void) { return profiler.result[PROFILER].freq_stats0_sum; };
u64 mifpro_get_freq_stats1_sum(void) { return profiler.result[PROFILER].freq_stats1_sum; };
u64 mifpro_get_freq_stats0_avg(void) { return profiler.result[PROFILER].freq_stats0_avg; };
u64 mifpro_get_freq_stats_ratio(void) { return profiler.result[PROFILER].freq_stats_ratio; };
u64 mifpro_get_llc_status(void) { return profiler.result[PROFILER].llc_status; };


struct private_fn_mif mif_pd_fn = {
	.get_stats0_sum	= &mifpro_get_freq_stats0_sum,
	.get_stats0_avg	= &mifpro_get_freq_stats0_avg,
	.get_stats1_sum	= &mifpro_get_freq_stats1_sum,
	.get_stats_ratio	= &mifpro_get_freq_stats_ratio,
	.get_llc_status = &mifpro_get_llc_status,
};

struct domain_fn mif_fn = {
	.get_table_cnt		= &mifpro_get_table_cnt,
	.get_freq_table		= &mifpro_get_freq_table,
	.get_max_freq		= &mifpro_get_max_freq,
	.get_min_freq		= &mifpro_get_min_freq,
	.get_freq		= &mifpro_get_freq,
	.get_power		= &mifpro_get_power,
	.get_power_change	= &mifpro_get_power_change,
	.get_active_pct		= &mifpro_get_active_pct,
	.get_temp		= &mifpro_get_temp,
	.set_margin		= &mifpro_set_margin,
	.update_mode		= &mifpro_update_mode,
};

/************************************************************************
 *			Gathering MIFFreq Information			*
 ************************************************************************/
//ktime_t * exynos_stats_get_mif_time_in_state(void);
extern u64 exynos_bcm_get_ccnt(unsigned int idx);
u32 mifpro_update_profile(int user)
{
	struct freq_cstate *fc = &profiler.fc;
	struct freq_cstate_snapshot *fc_snap = &profiler.fc_snap[user];
	struct freq_cstate_result *fc_result = &profiler.result[user].fc_result;
	struct mif_profile_result *result = &profiler.result[user];
	int i;
	u64 total_active_time = 0, diff_ccnt = 0, freq_stats2_sum = 0, freq_stats3_sum = 0;

#if defined(CONFIG_SOC_S5E9925_EVT0) || defined(CONFIG_SOC_S5E8825)
	u64 ccnt = 0;
	static u64 prev_ccnt = 0;
#else
	struct exynos_wow_profile *profile_in_state;
#endif

	profiler.cur_freq_idx = get_idx_from_freq(profiler.table, profiler.table_cnt,
			*(profiler.freq_infos.cur_freq), RELATION_LOW);
	profiler.max_freq_idx = get_idx_from_freq(profiler.table, profiler.table_cnt,
			exynos_pm_qos_request(profiler.freq_infos.pm_qos_class_max), RELATION_LOW);
	profiler.min_freq_idx = get_idx_from_freq(profiler.table, profiler.table_cnt,
			exynos_pm_qos_request(profiler.freq_infos.pm_qos_class), RELATION_HIGH);

#if defined(CONFIG_SOC_S5E9925_EVT0) || defined(CONFIG_SOC_S5E8825)
	// Update time in state and get tables from DVFS driver

	exynos_devfreq_get_profile(profiler.devfreq_type, fc->time, profiler.freq_stats);

	// calculate delta from previous status
	make_snapshot_and_time_delta(fc, fc_snap, fc_result, profiler.table_cnt);

	for (i = 0; i < 4; i++)
		calc_delta(result->freq_stats[i], profiler.freq_stats_snap[i], profiler.freq_stats[i], profiler.table_cnt);
	// Call to calc power
	compute_freq_cstate_result(profiler.table, fc_result, profiler.table_cnt,
					profiler.cur_freq_idx, result->avg_temp);

	ccnt = exynos_bcm_get_ccnt(0);

	diff_ccnt = ccnt - prev_ccnt;
	prev_ccnt = ccnt;
#else
	profile_in_state = kzalloc(sizeof(struct exynos_wow_profile) * profiler.table_cnt, GFP_KERNEL);
	exynos_devfreq_get_profile(profiler.devfreq_type, fc->time, profile_in_state);

	// calculate delta from previous status
	make_snapshot_and_time_delta(fc, fc_snap, fc_result, profiler.table_cnt);

	// Call to calc power
	compute_freq_cstate_result(profiler.table, fc_result, profiler.table_cnt,
					profiler.cur_freq_idx, result->avg_temp);
	// Calculate freq_stats array
	for (i = 0; i < profiler.table_cnt; i++) {
		result->freq_stats[0][i] = (profile_in_state[i].transfer_data - profiler.prev_wow_profile[i].transfer_data) >> 20;
		result->freq_stats[2][i] = (profile_in_state[i].nr_requests - profiler.prev_wow_profile[i].nr_requests);
		result->freq_stats[3][i] = (profile_in_state[i].mo_count - profiler.prev_wow_profile[i].mo_count);
		diff_ccnt += (profile_in_state[i].ccnt - profiler.prev_wow_profile[i].ccnt);
	}
	memcpy(profiler.prev_wow_profile, profile_in_state, sizeof(struct exynos_wow_profile) * profiler.table_cnt);
	kfree(profile_in_state);
	diff_ccnt /= 1000;
#endif
	for (i = 0 ; i < profiler.table_cnt; i++)
		fc->time[CLK_OFF][i] -= fc->time[CS_ACTIVE][i];

	result->freq_stats0_sum = 0;
	result->freq_stats1_sum = 0;
	fc_result->dyn_power = 0;

	for (i = 0; i < profiler.table_cnt; i++) {
		result->freq_stats0_sum += result->freq_stats[0][i];
		result->freq_stats1_sum += result->freq_stats[1][i];
		result->freq_stats0_avg += (result->freq_stats[0][i] << 40) / (profiler.table[i].freq / 1000);
		total_active_time += fc_result->time[CS_ACTIVE][i];
		freq_stats2_sum += result->freq_stats[2][i];
		freq_stats3_sum += result->freq_stats[3][i];
		fc_result->dyn_power += ((result->freq_stats[0][i] * profiler.table[i].dyn_cost) / fc_result->profile_time);
	}

	result->freq_stats0_sum = result->freq_stats0_sum * 1000000000 / fc_result->profile_time;
	result->freq_stats1_sum = result->freq_stats1_sum * 1000000000 / fc_result->profile_time;
	result->freq_stats0_avg = result->freq_stats0_avg / total_active_time;
	result->freq_stats_ratio = fc_result->profile_time * freq_stats3_sum / freq_stats2_sum / diff_ccnt;
	result->llc_status = llc_get_en();
	if (profiler.tz) {
		int temp = get_temp(profiler.tz);
		profiler.result[user].avg_temp = (temp + profiler.result[user].cur_temp) >> 1;
		profiler.result[user].cur_temp = temp;
	}

	return 0;
}

/************************************************************************
 *				INITIALIZATON				*
 ************************************************************************/
/* Initialize profiler data */
static int register_export_fn(u32 *max_freq, u32 *min_freq, u32 *cur_freq)
{
	struct exynos_devfreq_freq_infos *freq_infos = &profiler.freq_infos;

	exynos_devfreq_get_freq_infos(profiler.devfreq_type, freq_infos);

	*max_freq = freq_infos->max_freq;		/* get_org_max_freq(void) */
	*min_freq = freq_infos->min_freq;		/* get_org_min_freq(void) */
	*cur_freq = *freq_infos->cur_freq;		/* get_cur_freq(void)	  */
	profiler.table_cnt = freq_infos->max_state;	/* get_freq_table_cnt(void)  */
	/*
	// 2020
	profiler.fc.time[CS_ACTIVE] = exynos_stats_get_mif_time_in_state();

	// Olympus
	profiler.fc.time[CS_ACTIVE] = get_time_inf_freq(CS_ACTIVE);
	profiler.fc.time[CLKOFF] = get_time_in_freq(CLKOFF);
	*/

	return 0;
}

static int parse_dt(struct device_node *dn)
{
	int ret;

	/* necessary data */
	ret = of_property_read_u32(dn, "cal-id", &profiler.cal_id);
	if (ret)
		return -2;

	/* un-necessary data */
	ret = of_property_read_s32(dn, "profiler-id", &profiler.profiler_id);
	if (ret)
		profiler.profiler_id = -1;	/* Don't support profiler */

	ret = of_property_read_s32(dn, "devfreq-type", &profiler.devfreq_type);
	if (ret)
		profiler.devfreq_type = -1;	/* Don't support profiler */

	of_property_read_u32(dn, "power-coefficient", &profiler.dyn_pwr_coeff);
	of_property_read_u32(dn, "static-power-coefficient", &profiler.st_pwr_coeff);
//	of_property_read_string(dn, "tz-name", &profiler.tz_name);

	return 0;
}

static int init_profile_result(struct mif_profile_result *result, int size)
{
	int state;

	if (init_freq_cstate_result(&result->fc_result, size))
		return -ENOMEM;
	/* init private data */
	for (state = 0; state < 4; state++) {
		ktime_t *state_in_freq;
		state_in_freq = alloc_state_in_freq(size);
		if (!state_in_freq)
			return -ENOMEM;
		result->freq_stats[state] = state_in_freq;
	}

	return 0;
}

#ifdef CONFIG_EXYNOS_DEBUG_INFO
static void show_profiler_info(void)
{
	int idx;

	pr_info("================ mif domain ================\n");
	pr_info("min= %dKHz, max= %dKHz\n",
			profiler.table[profiler.table_cnt - 1].freq, profiler.table[0].freq);
	for (idx = 0; idx < profiler.table_cnt; idx++)
		pr_info("lv=%3d freq=%8d volt=%8d dyn_cost=%lld st_cost=%lld\n",
			idx, profiler.table[idx].freq, profiler.table[idx].volt,
			profiler.table[idx].dyn_cost,
			profiler.table[idx].st_cost);
	if (profiler.tz_name)
		pr_info("support temperature (tz_name=%s)\n", profiler.tz_name);
	if (profiler.profiler_id != -1)
		pr_info("support profiler domain(id=%d)\n", profiler.profiler_id);
}
#endif

static int exynos_mif_profiler_probe(struct platform_device *pdev)
{
	unsigned int org_max_freq, org_min_freq, cur_freq;
	int ret, idx;
#if defined(CONFIG_SOC_S5E9925_EVT0) || defined(CONFIG_SOC_S5E8825)
	int i;
#endif

	/* get node of device tree */
	if (!pdev->dev.of_node) {
		pr_err("mifpro: failed to get device treee\n");
		return -EINVAL;
	}
	profiler.root = pdev->dev.of_node;

	/* Parse data from Device Tree to init domain */
	ret = parse_dt(profiler.root);
	if (ret) {
		pr_err("mifpro: failed to parse dt(ret: %d)\n", ret);
		return -EINVAL;
	}

	register_export_fn(&org_max_freq, &org_min_freq, &cur_freq);

	/* init freq table */
	profiler.table = init_freq_table(NULL, profiler.table_cnt,
			profiler.cal_id, org_max_freq, org_min_freq,
			profiler.dyn_pwr_coeff, profiler.st_pwr_coeff,
			PWR_COST_CVV, PWR_COST_CVV);
	if (!profiler.table) {
		pr_err("mifpro: failed to init freq_table\n");
		return -EINVAL;
	}
	profiler.max_freq_idx = 0;
	profiler.min_freq_idx = profiler.table_cnt - 1;
	profiler.cur_freq_idx = get_idx_from_freq(profiler.table,
				profiler.table_cnt, cur_freq, RELATION_HIGH);

	if (init_freq_cstate(&profiler.fc, profiler.table_cnt))
			return -ENOMEM;

	/* init snapshot & result table */
	for (idx = 0; idx < NUM_OF_USER; idx++) {
		if (init_freq_cstate_snapshot(&profiler.fc_snap[idx],
						profiler.table_cnt))
			return -ENOMEM;

		if (init_profile_result(&profiler.result[idx], profiler.table_cnt))
			return -EINVAL;
	}
	
	profiler.prev_wow_profile = kzalloc(sizeof(struct exynos_wow_profile) * profiler.table_cnt, GFP_KERNEL);
	if (profiler.prev_wow_profile == NULL) {
		return -ENOMEM;
	}

	/* get thermal-zone to get temperature */
	if (profiler.tz_name)
		profiler.tz = init_temp(profiler.tz_name);

	if (profiler.tz)
		init_static_cost(profiler.table, profiler.table_cnt,
				1, profiler.root, profiler.tz);

	ret = exynos_profiler_register_domain(PROFILER_MIF, &mif_fn, &mif_pd_fn);

#ifdef CONFIG_EXYNOS_DEBUG_INFO
	show_profiler_info();
#endif

	return ret;
}

static const struct of_device_id exynos_mif_profiler_match[] = {
	{
		.compatible	= "samsung,exynos-mif-profiler",
	},
	{},
};
MODULE_DEVICE_TABLE(of, exynos_mif_profiler_match);

static struct platform_driver exynos_mif_profiler_driver = {
	.probe		= exynos_mif_profiler_probe,
	.driver	= {
		.name	= "exynos-mif-profiler",
		.owner	= THIS_MODULE,
		.of_match_table = exynos_mif_profiler_match,
	},
};

static int exynos_mif_profiler_init(void)
{
	return platform_driver_register(&exynos_mif_profiler_driver);
}
late_initcall(exynos_mif_profiler_init);

MODULE_DESCRIPTION("Exynos MIF Profiler v4");
MODULE_LICENSE("GPL");
