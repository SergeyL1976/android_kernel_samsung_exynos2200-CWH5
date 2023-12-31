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

#ifndef IS_CIS_3J1_H
#define IS_CIS_3J1_H

#include "is-cis.h"

#define SENSOR_3J1_MAX_WIDTH		(3968 + 0)
#define SENSOR_3J1_MAX_HEIGHT		(2736 + 0)

#define SENSOR_3J1_FINE_INTEGRATION_TIME_MIN                0x100
#define SENSOR_3J1_FINE_INTEGRATION_TIME_MAX                0x100
#define SENSOR_3J1_COARSE_INTEGRATION_TIME_MIN              0x2
#define SENSOR_3J1_COARSE_INTEGRATION_TIME_MAX_MARGIN       0x4
#define SENSOR_3J1_POST_INIT_SETTING_MAX	30

#define USE_GROUP_PARAM_HOLD	(0)

enum sensor_3j1_mode_enum {
	SENSOR_3J1_3648X2736_30FPS = 0,
	SENSOR_3J1_3968X2232_30FPS = 1,
	SENSOR_3J1_3024X2268_30FPS = 2,
	SENSOR_3J1_3280X1848_30FPS = 3,
	SENSOR_3J1_3280X2268_30FPS = 4,
	SENSOR_3J1_1824X1368_30FPS = 5,
	SENSOR_3J1_1988X1120_30FPS = 6,
	SENSOR_3J1_1520X1136_30FPS = 7,
	SENSOR_3J1_1640X924_30FPS = 8,
	SENSOR_3J1_3968X2232_60FPS = 9,
	SENSOR_3J1_1988X1120_120FPS = 10,
	SENSOR_3J1_912X684_120FPS = 11,
	SENSOR_3J1_MODE_MAX,
};
#endif

