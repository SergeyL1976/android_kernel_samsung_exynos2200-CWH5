/*
 * s2mf301-private.h - Voltage regulator driver for the s2mf301
 *
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __LINUX_MFD_S2MF301_PRIV_H
#define __LINUX_MFD_S2MF301_PRIV_H

#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/power/s2m_chg_manager.h>

#define MFD_DEV_NAME "s2mf301"

/*
 * Slave Address for the MFD
 * includes :
 * MUIC, AFC.
 * 1 bit right-shifted.
 */
#define I2C_ADDR_7C_SLAVE	((0x7C) >> 1)

/*
 * Slave Address for
 * Power meter
 * 1 bit right-shifted.
*/
#define I2C_ADDR_7E_SLAVE	((0x7E) >> 1)

/*
 * Slave Address for
 * Charger
 * 1 bit right-shifted.
*/
#define I2C_ADDR_7A_SLAVE	((0x7A) >> 1)

/*
 * Slave Address for
 * Fuel gauge
 * 1 bit right-shifted.
*/
#define I2C_ADDR_76_SLAVE	((0x76) >> 1)

/* Master Register Addr */
#define S2MF301_REG_IPINT		0x00
#define S2MF301_REG_IPINT_MASK		0x08
#define S2MF301_REG_PMICID		0xF5
#define S2MF301_REG_PMICID_MASK		0x0F
#define S2MF301_REG_INVALID		0xFF

/* IRQ */
enum s2mf301_irq_source {
#if IS_ENABLED(CONFIG_TOP_S2MF301)
    DC_INT,
    PM_RID_INT,
    CC_RID_INT,
#endif
#if IS_ENABLED(CONFIG_CHARGER_S2MF301)
	CHG_INT0,
	CHG_INT1,
	CHG_INT2,
	CHG_INT3,
	CHG_INT4,
#endif
#if IS_ENABLED(CONFIG_FUELGAUGE_S2MF301)
	FG_INT,
#endif
#if IS_ENABLED(CONFIG_LEDS_S2MF301_FLASH)
	FLED_INT,
#endif
#if IS_ENABLED(CONFIG_HV_MUIC_S2MF301_AFC)
	AFC_INT,
#endif
#if IS_ENABLED(CONFIG_MUIC_S2MF301)
	MUIC_INT,
#endif
#if IS_ENABLED(CONFIG_PM_S2MF301)
	PM_ADC_REQ_DONE1,
	PM_ADC_REQ_DONE2,
	PM_ADC_REQ_DONE3,
	PM_ADC_REQ_DONE4,
	PM_ADC_CHANGE_INT1,
	PM_ADC_CHANGE_INT2,
	PM_ADC_CHANGE_INT3,
	PM_ADC_CHANGE_INT4,
#endif
#if IS_ENABLED(CONFIG_REGULATOR_S2MF301)
	HBST_INT,
#endif
	S2MF301_IRQ_GROUP_NR,
};

#define S2MF301_NUM_IRQ_CHG_REGS	5
#define S2MF301_NUM_IRQ_FG_REGS	1
#define S2MF301_NUM_IRQ_LED_REGS	1
#define S2MF301_NUM_IRQ_AFC_REGS	1
#define S2MF301_NUM_IRQ_MUIC_REGS	1
#define S2MF301_NUM_IRQ_PM_REGS		4
#define S2MF301_NUM_IRQ_HBST_REGS	1

#define S2MF301_IRQSRC_MUIC	(1 << 0)
#define S2MF301_IRQSRC_FLED	(1 << 1)
#define S2MF301_IRQSRC_CHG	(1 << 2)
#define S2MF301_IRQSRC_AFC	(1 << 3)
#define S2MF301_IRQSRC_PM	(1 << 4)
#define S2MF301_IRQSRC_DC	(1 << 5)
#define S2MF301_IRQSRC_FG	(1 << 6)
#define S2MF301_IRQSRC_RID	(1 << 7)

enum s2mf301_irq {
#if IS_ENABLED(CONFIG_TOP_S2MF301)
    S2MF301_TOP_DC_IRQ_RAMP_UP_DONE,
    S2MF301_TOP_DC_IRQ_RAMP_UP_FAIL,
    S2MF301_TOP_DC_IRQ_THERMAL_CONTROL,
    S2MF301_TOP_DC_IRQ_CHARGING_STATE_CHAGNE,
    S2MF301_TOP_DC_IRQ_CHARGING_DONE,

    S2MF301_TOP_PM_RID_IRQ_RID_56K,
    S2MF301_TOP_PM_RID_IRQ_RID_255K,
    S2MF301_TOP_PM_RID_IRQ_RID_301K,
    S2MF301_TOP_PM_RID_IRQ_RID_523K,
    S2MF301_TOP_PM_RID_IRQ_RID_619KK,
    S2MF301_TOP_PM_RID_IRQ_RID_OTG,
    S2MF301_TOP_PM_RID_IRQ_RID_DETACH,
    S2MF301_TOP_PM_RID_IRQ_RID_ATTACH,

    S2MF301_TOP_CC_RID_IRQ_RID_255K,
    S2MF301_TOP_CC_RID_IRQ_RID_301K,
    S2MF301_TOP_CC_RID_IRQ_RID_523K,
    S2MF301_TOP_CC_RID_IRQ_RID_619K,
    S2MF301_TOP_CC_RID_IRQ_RID_OTG,
    S2MF301_TOP_CC_RID_IRQ_RID_DETACH,
    S2MF301_TOP_CC_RID_IRQ_RID_ATTACH,
#endif
#if IS_ENABLED(CONFIG_CHARGER_S2MF301)
	S2MF301_CHG0_IRQ_CHGIN_UVLOB,
	S2MF301_CHG0_IRQ_CHGIN2BATB,
	S2MF301_CHG0_IRQ_CHGIN_OVP,
	S2MF301_CHG0_IRQ_VBUS_DET,
	S2MF301_CHG0_IRQ_BATID,

	S2MF301_CHG1_IRQ_RECHARGING,
	S2MF301_CHG1_IRQ_DONE,
	S2MF301_CHG1_IRQ_TOPOFF,
	S2MF301_CHG1_IRQ_CV,
	S2MF301_CHG1_IRQ_SC,
	S2MF301_CHG1_IRQ_LC,
	S2MF301_CHG1_IRQ_TRICKLE,
	S2MF301_CHG1_IRQ_PRECHG,

	S2MF301_CHG2_IRQ_IVR,
	S2MF301_CHG2_IRQ_ICR,
	S2MF301_CHG2_IRQ_VOLTAGE_LOOP,
	S2MF301_CHG2_IRQ_SC_CC_LOOP,
	S2MF301_CHG2_IRQ_BST_ON,
	S2MF301_CHG2_IRQ_OTG_ON_OFF,
	S2MF301_CHG2_IRQ_BST_LBAT,
	S2MF301_CHG2_IRQ_OTG_TO_BAT,

	S2MF301_CHG3_IRQ_TOPOFF_TIMER_FAULT,
	S2MF301_CHG3_IRQ_COOL_FAST_CHG_TIMER_FAULT,
	S2MF301_CHG3_IRQ_PRE_TRICKLE_CHG_TIMER_FAULT,
	S2MF301_CHG3_IRQ_BAT_OCP,
	S2MF301_CHG3_IRQ_WDT_AP_RESET,
	S2MF301_CHG3_IRQ_WDT_SUSPEND,
	S2MF301_CHG3_IRQ_VSYS_OVP,
	S2MF301_CHG3_IRQ_VSYS_SCP,

	S2MF301_CHG4_IRQ_QBAT_OFF,
	S2MF301_CHG4_IRQ_TFB,
	S2MF301_CHG4_IRQ_TSD,
#endif
#if IS_ENABLED(CONFIG_FUELGAUGE_S2MF301)
	S2MF301_FG_IRQ_LOW_SOC,
	S2MF301_FG_IRQ_LOW_VBAT,
	S2MF301_FG_IRQ_HIGH_TEMP,
	S2MF301_FG_IRQ_LOW_TEMP,
#endif
#if IS_ENABLED(CONFIG_LEDS_S2MF301)
	S2MF301_FLED_IRQ_C2F_Vbyp_ovp_prot,
	S2MF301_FLED_IRQ_C2F_Vbyp_OK_Warning,
	S2MF301_FLED_IRQ_TORCH_ON,
	S2MF301_FLED_IRQ_LED_ON_TA_Detach,
	S2MF301_FLED_IRQ_CH1_ON,
	S2MF301_FLED_IRQ_OPEN_CH1,
	S2MF301_FLED_IRQ_SHORT_CH1,
#endif
#if IS_ENABLED(CONFIG_HV_MUIC_S2MF301_AFC)
	S2MF301_AFC_IRQ_AFC_LOOP,
	S2MF301_AFC_IRQ_VDNMon,
	S2MF301_AFC_IRQ_DNRes,
	S2MF301_AFC_IRQ_MPNack,
	S2MF301_AFC_IRQ_MRxBufOw,
	S2MF301_AFC_IRQ_MRxTrf,
	S2MF301_AFC_IRQ_MRxPerr,
	S2MF301_AFC_IRQ_MRxRdy,
#endif
#if IS_ENABLED(CONFIG_MUIC_S2MF301)
	S2MF301_MUIC_IRQ_ATTATCH,
	S2MF301_MUIC_IRQ_DETACH,
	S2MF301_MUIC_IRQ_RESERVED,
	S2MF301_MUIC_IRQ_USB_OVP,
	S2MF301_MUIC_IRQ_VBUS_ON,
	S2MF301_MUIC_IRQ_VBUS_OFF,
	S2MF301_MUIC_IRQ_USB_Killer,
	S2MF301_MUIC_IRQ_GP_OVP,
#endif
#if IS_ENABLED(CONFIG_PM_S2MF301)
	S2MF301_PM_ADC_REQ_DONE1_VCC2UP,
	S2MF301_PM_ADC_REQ_DONE1_VCC1UP,
	S2MF301_PM_ADC_REQ_DONE1_VBATUP,
	S2MF301_PM_ADC_REQ_DONE1_VSYSUP,
	S2MF301_PM_ADC_REQ_DONE1_VBYPUP,
	S2MF301_PM_ADC_REQ_DONE1_VWCINUP,
	S2MF301_PM_ADC_REQ_DONE1_VCHGINUP,
	S2MF301_PM_ADC_REQ_DONE2_GPADC3UP,
	S2MF301_PM_ADC_REQ_DONE2_GPADC2UP,
	S2MF301_PM_ADC_REQ_DONE2_GPADC1UP,
	S2MF301_PM_ADC_REQ_DONE2_ITXUP,
	S2MF301_PM_ADC_REQ_DONE2_IOTGUP,
	S2MF301_PM_ADC_REQ_DONE2_IWCINUP,
	S2MF301_PM_ADC_REQ_DONE2_ICHGINUP,
	S2MF301_PM_ADC_CHANGE_INT1_VCC2UP,
	S2MF301_PM_ADC_CHANGE_INT1_VCC1UP,
	S2MF301_PM_ADC_CHANGE_INT1_VBATUP,
	S2MF301_PM_ADC_CHANGE_INT1_VSYSUP,
	S2MF301_PM_ADC_CHANGE_INT1_VBYPUP,
	S2MF301_PM_ADC_CHANGE_INT1_VWCINUP,
	S2MF301_PM_ADC_CHANGE_INT1_VCHGINUP,
	S2MF301_PM_ADC_CHANGE_INT2_GPADC3UP,
	S2MF301_PM_ADC_CHANGE_INT2_GPADC2UP,
	S2MF301_PM_ADC_CHANGE_INT2_GPADC1UP,
	S2MF301_PM_ADC_CHANGE_INT2_ITXUP,
	S2MF301_PM_ADC_CHANGE_INT2_IOTGUP,
	S2MF301_PM_ADC_CHANGE_INT2_IWCINUP,
	S2MF301_PM_ADC_CHANGE_INT2_ICHGINUP,
#endif
#if IS_ENABLED(CONFIG_REGULATOR_S2MF301)
	S2MF301_HBST_IRQ_OFF,
	S2MF301_HBST_IRQ_ON,
	S2MF301_HBST_IRQ_SCP,
#endif
	S2MF301_IRQ_NR,
};

struct s2mf301_platform_data {
	/* IRQ */
	int irq_base;
	int irq_gpio;
	bool wakeup;
};

struct s2mf301_dev {
	struct device *dev;
	struct i2c_client *i2c;		/* Slave addr = 0x74 */
	struct i2c_client *muic;	/* Slave addr = 0x7C */
	struct i2c_client *pm;		/* Slave addr = 0x7E */
	struct i2c_client *chg;		/* Slave addr = 0x7A */
	struct i2c_client *fg;
	struct mutex i2c_lock;

	int type;

	int irq;
	int irq_base;
	int irq_gpio;
	bool wakeup;
	struct mutex irqlock;
	int irq_masks_cur[S2MF301_IRQ_GROUP_NR];
	int irq_masks_cache[S2MF301_IRQ_GROUP_NR];

	/* pmic VER/REV register */
	u8 pmic_rev;	/* pmic Rev */
	u8 pmic_ver;	/* pmic version */

	struct s2mf301_platform_data *pdata;
};

enum s2mf301_types {
	TYPE_S2MF301,
};

extern int s2mf301_irq_init(struct s2mf301_dev *s2mf301);
extern void s2mf301_irq_exit(struct s2mf301_dev *s2mf301);

/* s2mf301 shared i2c API function */
extern int s2mf301_read_reg(struct i2c_client *i2c, u8 reg, u8 *dest);
extern int s2mf301_bulk_read(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int s2mf301_write_reg(struct i2c_client *i2c, u8 reg, u8 value);
extern int s2mf301_bulk_write(struct i2c_client *i2c, u8 reg, int count,
				u8 *buf);
extern int s2mf301_write_word(struct i2c_client *i2c, u8 reg, u16 value);
extern int s2mf301_read_word(struct i2c_client *i2c, u8 reg);

extern int s2mf301_update_reg(struct i2c_client *i2c, u8 reg, u8 val, u8 mask);

#endif /* __LINUX_MFD_S2MF301_PRIV_H */

