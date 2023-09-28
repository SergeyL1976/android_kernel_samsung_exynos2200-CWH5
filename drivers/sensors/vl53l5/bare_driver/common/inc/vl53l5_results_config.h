/*******************************************************************************
* Copyright (c) 2021, STMicroelectronics - All Rights Reserved
*
* This file is part of VL53L5 Kernel Driver and is dual licensed,
* either 'STMicroelectronics Proprietary license'
* or 'BSD 3-clause "New" or "Revised" License' , at your option.
*
********************************************************************************
*
* 'STMicroelectronics Proprietary license'
*
********************************************************************************
*
* License terms: STMicroelectronics Proprietary in accordance with licensing
* terms at www.st.com/sla0081
*
* STMicroelectronics confidential
* Reproduction and Communication of this document is strictly prohibited unless
* specifically authorized in writing by STMicroelectronics.
*
*
********************************************************************************
*
* Alternatively, VL53L5 Kernel Driver may be distributed under the terms of
* 'BSD 3-clause "New" or "Revised" License', in which case the following
* provisions apply instead of the ones mentioned above :
*
********************************************************************************
*
* License terms: BSD 3-clause "New" or "Revised" License.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
*
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software
* without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*
*******************************************************************************/

#ifndef __VL53L5_RESULTS_CONFIG_H__
#define __VL53L5_RESULTS_CONFIG_H__

#include "vl53l5_results_build_config.h"
#include "dci_defs.h"
#include "dci_ui_memory_defs.h"
#include "dci_size.h"
#include "dci_ui_size.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined(VL53L5_AMB_RATE_KCPS_PER_SPAD_ON) || \
	defined(VL53L5_EFFECTIVE_SPAD_COUNT_ON) || \
	defined(VL53L5_AMB_DMAX_MM_ON) || \
	defined(VL53L5_SILICON_TEMP_DEGC_START_ON) || \
	defined(VL53L5_SILICON_TEMP_DEGC_END_ON) || \
	defined(VL53L5_NO_OF_TARGETS_ON) || defined(VL53L5_ZONE_ID_ON) || \
	defined(VL53L5_SEQUENCE_IDX_ON))
#define VL53L5_PER_ZONE_RESULTS_ON
#endif

#if (defined(VL53L5_PEAK_RATE_KCPS_PER_SPAD_ON) || \
	defined(VL53L5_MEDIAN_PHASE_ON) || \
	defined(VL53L5_RATE_SIGMA_KCPS_PER_SPAD_ON) || \
	defined(VL53L5_TARGET_ZSCORE_ON) || defined(VL53L5_RANGE_SIGMA_MM_ON) \
	|| defined(VL53L5_MEDIAN_RANGE_MM_ON) || \
	defined(VL53L5_START_RANGE_MM_ON) || defined(VL53L5_END_RANGE_MM_ON) \
	|| defined(VL53L5_MIN_RANGE_DELTA_MM_ON) || \
	defined(VL53L5_MAX_RANGE_DELTA_MM_ON) || \
	defined(VL53L5_TARGET_REFLECTANCE_EST_PC_ON) || \
	defined(VL53L5_TARGET_STATUS_ON))
#define VL53L5_PER_TGT_RESULTS_ON
#endif

#if (defined(VL53L5_SHARPENER_GROUP_INDEX_ON) || \
	defined(VL53L5_SHARPENER_CONFIDENCE_ON))
#define VL53L5_SHARPENER_TARGET_DATA_ON
#endif

#if (defined(VL53L5_ZONE_THRESH_STATUS_BYTES_ON))
#define VL53L5_ZONE_THRESH_STATUS_ON
#endif

#ifdef VL53L5_SILICON_TEMP_DATA_ON
#define VL53L5_SILICON_TEMP_DATA_BLOCK_SZ \
	(VL53L5_SILICON_TEMPERATURE_DATA_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_SILICON_TEMP_DATA_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_ZONE_CFG_ON
#define VL53L5_ZONE_CFG_BLOCK_SZ \
	(VL53L5_ZONE_CFG_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_ZONE_CFG_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_META_DATA_ON
#define VL53L5_META_DATA_BLOCK_SZ \
	(VL53L5_BUF_META_DATA_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_META_DATA_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_COMMON_DATA_ON
#define VL53L5_COMMON_DATA_BLOCK_SZ \
	(VL53L5_RNG_COMMON_DATA_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_COMMON_DATA_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_RNG_TIMING_DATA_ON
#define VL53L5_RNG_TIMING_DATA_BLOCK_SZ \
	(VL53L5_RNG_TIMING_DATA_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_RNG_TIMING_DATA_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_AMB_RATE_KCPS_PER_SPAD_ON
#define VL53L5_AMB_RATE_KCPS_PER_SPAD_BLOCK_SZ \
	(VL53L5_RPZD_AMB_RATE_KCPS_PER_SPAD_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_AMB_RATE_KCPS_PER_SPAD_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_EFFECTIVE_SPAD_COUNT_ON
#define VL53L5_EFFECTIVE_SPAD_COUNT_BLOCK_SZ \
	(VL53L5_RPZD_RNG_EFFECTIVE_SPAD_COUNT_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_EFFECTIVE_SPAD_COUNT_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_AMB_DMAX_MM_ON
#define VL53L5_AMB_DMAX_MM_BLOCK_SZ \
	(VL53L5_RPZD_AMB_DMAX_MM_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_AMB_DMAX_MM_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_SILICON_TEMP_DEGC_START_ON
#define VL53L5_SILICON_TEMP_DEGC_START_BLOCK_SZ \
	(VL53L5_RPZD_SILICON_TEMP_DEGC_START_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_SILICON_TEMP_DEGC_START_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_SILICON_TEMP_DEGC_END_ON
#define VL53L5_SILICON_TEMP_DEGC_END_BLOCK_SZ \
	(VL53L5_RPZD_SILICON_TEMP_DEGC_END_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_SILICON_TEMP_DEGC_END_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_NO_OF_TARGETS_ON
#define VL53L5_NO_OF_TARGETS_BLOCK_SZ \
	(VL53L5_RPZD_RNG_NO_OF_TARGETS_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_NO_OF_TARGETS_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_ZONE_ID_ON
#define VL53L5_ZONE_ID_BLOCK_SZ \
	(VL53L5_RPZD_RNG_ZONE_ID_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_ZONE_ID_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_SEQUENCE_IDX_ON
#define VL53L5_SEQUENCE_IDX_BLOCK_SZ \
	(VL53L5_RPZD_RNG_SEQUENCE_IDX_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_SEQUENCE_IDX_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_PEAK_RATE_KCPS_PER_SPAD_ON
#define VL53L5_PEAK_RATE_KCPS_PER_SPAD_BLOCK_SZ \
	(VL53L5_RPTD_PEAK_RATE_KCPS_PER_SPAD_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_PEAK_RATE_KCPS_PER_SPAD_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_MEDIAN_PHASE_ON
#define VL53L5_MEDIAN_PHASE_BLOCK_SZ \
	(VL53L5_RPTD_MEDIAN_PHASE_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_MEDIAN_PHASE_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_RATE_SIGMA_KCPS_PER_SPAD_ON
#define VL53L5_RATE_SIGMA_KCPS_PER_SPAD_BLOCK_SZ \
	(VL53L5_RPTD_RATE_SIGMA_KCPS_PER_SPAD_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_RATE_SIGMA_KCPS_PER_SPAD_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_TARGET_ZSCORE_ON
#define VL53L5_TARGET_ZSCORE_BLOCK_SZ \
	(VL53L5_RPTD_TARGET_ZSCORE_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_TARGET_ZSCORE_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_RANGE_SIGMA_MM_ON
#define VL53L5_RANGE_SIGMA_MM_BLOCK_SZ \
	(VL53L5_RPTD_RANGE_SIGMA_MM_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_RANGE_SIGMA_MM_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_MEDIAN_RANGE_MM_ON
#define VL53L5_MEDIAN_RANGE_MM_BLOCK_SZ \
	(VL53L5_RPTD_MEDIAN_RANGE_MM_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_MEDIAN_RANGE_MM_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_START_RANGE_MM_ON
#define VL53L5_START_RANGE_MM_BLOCK_SZ \
	(VL53L5_RPTD_START_RANGE_MM_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_START_RANGE_MM_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_END_RANGE_MM_ON
#define VL53L5_END_RANGE_MM_BLOCK_SZ \
	(VL53L5_RPTD_END_RANGE_MM_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_END_RANGE_MM_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_MIN_RANGE_DELTA_MM_ON
#define VL53L5_MIN_RANGE_DELTA_MM_BLOCK_SZ \
	(VL53L5_RPTD_MIN_RANGE_DELTA_MM_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_MIN_RANGE_DELTA_MM_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_MAX_RANGE_DELTA_MM_ON
#define VL53L5_MAX_RANGE_DELTA_MM_BLOCK_SZ \
	(VL53L5_RPTD_MAX_RANGE_DELTA_MM_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_MAX_RANGE_DELTA_MM_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_TARGET_REFLECTANCE_EST_PC_ON
#define VL53L5_TARGET_REFLECTANCE_EST_PC_BLOCK_SZ \
	(VL53L5_RPTD_TARGET_REFLECTANCE_EST_PC_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_TARGET_REFLECTANCE_EST_PC_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_TARGET_STATUS_ON
#define VL53L5_TARGET_STATUS_BLOCK_SZ \
	(VL53L5_RPTD_TARGET_STATUS_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_TARGET_STATUS_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_REF_TIMING_DATA_ON
#define VL53L5_REF_TIMING_DATA_BLOCK_SZ \
	(VL53L5_RNG_TIMING_DATA_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_REF_TIMING_DATA_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_REF_CHANNEL_DATA_ON
#define VL53L5_REF_CHANNEL_DATA_BLOCK_SZ \
	(VL53L5_REF_CHANNEL_DATA_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_REF_CHANNEL_DATA_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_REF_TARGET_DATA_ON
#define VL53L5_REF_TARGET_DATA_BLOCK_SZ \
	(VL53L5_REF_TARGET_DATA_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_REF_TARGET_DATA_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_SHARPENER_GROUP_INDEX_ON
#define VL53L5_SHARPENER_GROUP_INDEX_BLOCK_SZ \
	(VL53L5_STD_SHARPENER_GROUP_INDEX_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_SHARPENER_GROUP_INDEX_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_SHARPENER_CONFIDENCE_ON
#define VL53L5_SHARPENER_CONFIDENCE_BLOCK_SZ \
	(VL53L5_STD_SHARPENER_CONFIDENCE_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_SHARPENER_CONFIDENCE_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_ZONE_THRESH_STATUS_BYTES_ON
#define VL53L5_ZONE_THRESH_STATUS_BYTES_BLOCK_SZ \
	(VL53L5_ZTSA_ZONE_THRESH_STATUS_BYTES_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_ZONE_THRESH_STATUS_BYTES_BLOCK_SZ \
	0
#endif

#ifdef VL53L5_DYN_XTALK_OP_PERSISTENT_DATA_ON
#define VL53L5_DYN_XTALK_OP_PERSISTENT_DATA_BLOCK_SZ \
	(VL53L5_DYN_XTALK_PERSISTENT_DATA_SZ \
	 + VL53L5_DCI_UI_PACKED_DATA_BH_SZ)
#else
#define VL53L5_DYN_XTALK_OP_PERSISTENT_DATA_BLOCK_SZ \
	0
#endif

#define VL53L5_DUMMY_BYTES_SZ \
	VL53L5_CONFIG_DUMMY_BYTES_SZ

#define VL53L5_RESULTS_PACKET_META_SIZE \
	(VL53L5_DCI_UI_DEV_INFO_SZ \
	 + VL53L5_DUMMY_BYTES_SZ \
	 + (2 * VL53L5_DCI_UI_RNG_DATA_HEADER_SZ) \
	 + (3 * VL53L5_DCI_UI_PACKED_DATA_BH_SZ))

#define VL53L5_RESULTS_TOTAL_SIZE_BYTES \
	(VL53L5_RESULTS_PACKET_META_SIZE \
	 + VL53L5_SILICON_TEMP_DATA_BLOCK_SZ \
	 + VL53L5_ZONE_CFG_BLOCK_SZ \
	 + VL53L5_META_DATA_BLOCK_SZ \
	 + VL53L5_COMMON_DATA_BLOCK_SZ \
	 + VL53L5_RNG_TIMING_DATA_BLOCK_SZ \
	 + VL53L5_AMB_RATE_KCPS_PER_SPAD_BLOCK_SZ \
	 + VL53L5_EFFECTIVE_SPAD_COUNT_BLOCK_SZ \
	 + VL53L5_AMB_DMAX_MM_BLOCK_SZ \
	 + VL53L5_SILICON_TEMP_DEGC_START_BLOCK_SZ \
	 + VL53L5_SILICON_TEMP_DEGC_END_BLOCK_SZ \
	 + VL53L5_NO_OF_TARGETS_BLOCK_SZ \
	 + VL53L5_ZONE_ID_BLOCK_SZ \
	 + VL53L5_SEQUENCE_IDX_BLOCK_SZ \
	 + VL53L5_PEAK_RATE_KCPS_PER_SPAD_BLOCK_SZ \
	 + VL53L5_MEDIAN_PHASE_BLOCK_SZ \
	 + VL53L5_RATE_SIGMA_KCPS_PER_SPAD_BLOCK_SZ \
	 + VL53L5_TARGET_ZSCORE_BLOCK_SZ \
	 + VL53L5_RANGE_SIGMA_MM_BLOCK_SZ \
	 + VL53L5_MEDIAN_RANGE_MM_BLOCK_SZ \
	 + VL53L5_START_RANGE_MM_BLOCK_SZ \
	 + VL53L5_END_RANGE_MM_BLOCK_SZ \
	 + VL53L5_MIN_RANGE_DELTA_MM_BLOCK_SZ \
	 + VL53L5_MAX_RANGE_DELTA_MM_BLOCK_SZ \
	 + VL53L5_TARGET_REFLECTANCE_EST_PC_BLOCK_SZ \
	 + VL53L5_TARGET_STATUS_BLOCK_SZ \
	 + VL53L5_REF_TIMING_DATA_BLOCK_SZ \
	 + VL53L5_REF_CHANNEL_DATA_BLOCK_SZ \
	 + VL53L5_REF_TARGET_DATA_BLOCK_SZ \
	 + VL53L5_SHARPENER_GROUP_INDEX_BLOCK_SZ \
	 + VL53L5_SHARPENER_CONFIDENCE_BLOCK_SZ \
	 + VL53L5_ZONE_THRESH_STATUS_BYTES_BLOCK_SZ \
	 + VL53L5_DYN_XTALK_OP_PERSISTENT_DATA_BLOCK_SZ)

#ifdef __cplusplus
}
#endif

#endif
