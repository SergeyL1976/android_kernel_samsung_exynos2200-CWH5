/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Samsung Exynos SoC series Sensor driver
 *
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef IS_CIS_IMX563_H
#define IS_CIS_IMX563_H

#include "is-cis.h"

#ifdef USE_CAMERA_IMX563_4000X3000
#define SENSOR_IMX563_MAX_WIDTH			(4000 + 0)
#define SENSOR_IMX563_MAX_HEIGHT		(3000 + 0)
#else
#define SENSOR_IMX563_MAX_WIDTH			(4032 + 0)
#define SENSOR_IMX563_MAX_HEIGHT		(3024 + 0)
#endif

#define SENSOR_IMX563_FINE_INTEGRATION_TIME_MIN				0x100
#define SENSOR_IMX563_FINE_INTEGRATION_TIME_MAX				0x100
#define SENSOR_IMX563_COARSE_INTEGRATION_TIME_MIN			0xA
#define SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN		0x60

#define USE_GROUP_PARAM_HOLD	(0)

#ifdef USE_CAMERA_IMX563_4000X3000
enum sensor_imx563_mode_enum {
	SENSOR_IMX563_4000X3000_30FPS = 0, //7b_1
	SENSOR_IMX563_4000X3000_60FPS = 1, //7a_2
	SENSOR_IMX563_4000X2252_30FPS = 2, //8b_1
	SENSOR_IMX563_4000X2252_60FPS = 3, //9b_1
	SENSOR_IMX563_1984X1488_30FPS = 4, //10_3
	SENSOR_IMX563_3328X1872_120FPS = 5, //15_2
	SENSOR_IMX563_1008X756_120FPS = 6, //11_3
	SENSOR_IMX563_1984X1488_240FPS = 7, //12_2
	SENSOR_IMX563_2016X1136_480FPS = 8, //13_2
	SENSOR_IMX563_1280X720_480FPS = 9, //16_2
	SENSOR_IMX563_2000X1500_120FPS = 10, //17_3
	SENSOR_IMX563_2800X2100_30FPS = 11, //7b_1_t1
	SENSOR_IMX563_2800X2100_24FPS = 12, //7b_1_t2
	SENSOR_IMX563_MODE_MAX,
};

static bool sensor_imx563_support_wdr[] = {
	false, //SENSOR_IMX563_4000X3000_30FPS = 0,
	false, //SENSOR_IMX563_4000X3000_60FPS = 1,
	false, //SENSOR_IMX563_4000X2252_30FPS = 2,
	false, //SENSOR_IMX563_4000X2252_60FPS = 3,
	false, //SENSOR_IMX563_1984X1488_30FPS = 4,
	false, //SENSOR_IMX563_3328X1872_120FPS = 5,
	false, //SENSOR_IMX563_1008X756_120FPS = 6,
	false, //SENSOR_IMX563_1984X1488_240FPS = 7,
	false, //SENSOR_IMX563_2016X1136_480FPS = 8,
	false, //SENSOR_IMX563_1280X720_480FPS = 9,
	false, //SENSOR_IMX563_2000X1500_120FPS = 10,
	false, //SENSOR_IMX563_2800X2100_30FPS = 11,
	false, //SENSOR_IMX563_2800X2100_24FPS = 12,
};

static u32 sensor_imx563_integration_max_margin[] = {
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_4000X3000_30FPS = 0,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_4000X3000_60FPS = 1,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_4000X2252_30FPS = 2,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_4000X2252_60FPS = 3,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_1984X1488_30FPS = 4,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_3328X1872_120FPS = 5,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_1008X756_120FPS = 6,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_1984X1488_240FPS = 7,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_2016X1136_480FPS = 8,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_1280X720_480FPS = 9,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_2000X1500_120FPS = 10,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_2800X2100_30FPS = 11,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_2800X2100_24FPS = 12,
};
#else
enum sensor_imx563_mode_enum {
	SENSOR_IMX563_4032X3024_30FPS = 0, //1b_2
	SENSOR_IMX563_4032X3024_60FPS = 1, //1a_3
	SENSOR_IMX563_4032X2268_30FPS = 2, //2b_2
	SENSOR_IMX563_4032X2268_60FPS = 3, //3b_2
	SENSOR_IMX563_3328X1872_120FPS = 4, //15_3
	SENSOR_IMX563_2016X1512_120FPS = 5, //4_3
	SENSOR_IMX563_2016X1134_120FPS = 6, //5_3
	SENSOR_IMX563_2016X1134_240FPS = 7, //6_3
	SENSOR_IMX563_MODE_MAX,
};

static bool sensor_imx563_support_wdr[] = {
	false, //SENSOR_IMX563_4032X3024_30FPS = 0,
	false, //SENSOR_IMX563_4032X3024_60FPS = 1,
	false, //SENSOR_IMX563_4032X2268_30FPS = 2,
	false, //SENSOR_IMX563_4032X2268_60FPS = 3,
	false, //SENSOR_IMX563_3328X1872_120FPS = 4,
	false, //SENSOR_IMX563_2016X1512_120FPS = 5,
	false, //SENSOR_IMX563_2016X1134_120FPS = 6,
	false, //SENSOR_IMX563_2016X1134_240FPS = 7,
};

static u32 sensor_imx563_integration_max_margin[] = {
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_4032X3024_30FPS = 0,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_4032X3024_60FPS = 1,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_4032X2268_30FPS = 2,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_4032X2268_60FPS = 3,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_3328X1872_120FPS = 4,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_2016X1512_120FPS = 5,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_2016X1134_120FPS = 6,
	SENSOR_IMX563_COARSE_INTEGRATION_TIME_MAX_MARGIN, //SENSOR_IMX563_2016X1134_240FPS = 7,
};
#endif

static const u32 sensor_imx563_cis_dual_master_settings[] = {
	0x3036,	0x00,	0x01,
	0x3030,	0x01,	0x01,
	0x3020,	0x01,	0x01,
	0x3031,	0x01,	0x01,
	0x3032,	0x07,	0x01,
	0x3033,	0x00,	0x01,
};

static const u32 sensor_imx563_cis_dual_master_settings_size =
	ARRAY_SIZE(sensor_imx563_cis_dual_master_settings);

int sensor_imx563_cis_stream_on(struct v4l2_subdev *subdev);
int sensor_imx563_cis_stream_off(struct v4l2_subdev *subdev);
#endif