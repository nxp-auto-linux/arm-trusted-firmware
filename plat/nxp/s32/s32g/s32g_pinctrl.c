/*
 * Copyright 2019-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/mmio.h>
#include <assert.h>
#include "s32g_pinctrl.h"

/* GPIOs for External Wakeup Sources */
#define SIUL2_PC_11_MSCR_S32_G1_WKUP0	43
#define SIUL2_PJ_02_MSCR_S32_G1_WKUP1	146
#define SIUL2_PJ_04_MSCR_S32_G1_WKUP2	148
#define SIUL2_PJ_06_MSCR_S32_G1_WKUP3	150
#define SIUL2_PJ_08_MSCR_S32_G1_WKUP4	152
#define SIUL2_PJ_10_MSCR_S32_G1_WKUP5	154
#define SIUL2_PJ_12_MSCR_S32_G1_WKUP6	156
#define SIUL2_PJ_14_MSCR_S32_G1_WKUP7	158
#define SIUL2_PK_00_MSCR_S32_G1_WKUP8	160
#define SIUL2_PK_02_MSCR_S32_G1_WKUP9	162
#define SIUL2_PK_04_MSCR_S32_G1_WKUP10	164
#define SIUL2_PK_06_MSCR_S32_G1_WKUP11	166
#define SIUL2_PK_08_MSCR_S32_G1_WKUP12	168
#define SIUL2_PK_10_MSCR_S32_G1_WKUP13	170
#define SIUL2_PK_12_MSCR_S32_G1_WKUP14	172
#define SIUL2_PK_14_MSCR_S32_G1_WKUP15	174
#define SIUL2_PL_00_MSCR_S32_G1_WKUP16	176
#define SIUL2_PC_04_MSCR_S32_G1_WKUP17	36
#define SIUL2_PC_06_MSCR_S32_G1_WKUP18	38
#define SIUL2_PL_07_MSCR_S32_G1_WKUP19	183
#define SIUL2_PL_03_MSCR_S32_G1_WKUP20	179
#define SIUL2_PL_06_MSCR_S32_G1_WKUP21	182
#define SIUL2_PA_10_MSCR_S32_G1_WKUP22	10

static const uint16_t wkpu_gpio[] = {
	SIUL2_PC_11_MSCR_S32_G1_WKUP0,
	SIUL2_PJ_02_MSCR_S32_G1_WKUP1,
	SIUL2_PJ_04_MSCR_S32_G1_WKUP2,
	SIUL2_PJ_06_MSCR_S32_G1_WKUP3,
	SIUL2_PJ_08_MSCR_S32_G1_WKUP4,
	SIUL2_PJ_10_MSCR_S32_G1_WKUP5,
	SIUL2_PJ_12_MSCR_S32_G1_WKUP6,
	SIUL2_PJ_14_MSCR_S32_G1_WKUP7,
	SIUL2_PK_00_MSCR_S32_G1_WKUP8,
	SIUL2_PK_02_MSCR_S32_G1_WKUP9,
	SIUL2_PK_04_MSCR_S32_G1_WKUP10,
	SIUL2_PK_06_MSCR_S32_G1_WKUP11,
	SIUL2_PK_08_MSCR_S32_G1_WKUP12,
	SIUL2_PK_10_MSCR_S32_G1_WKUP13,
	SIUL2_PK_12_MSCR_S32_G1_WKUP14,
	SIUL2_PK_14_MSCR_S32_G1_WKUP15,
	SIUL2_PL_00_MSCR_S32_G1_WKUP16,
	SIUL2_PC_04_MSCR_S32_G1_WKUP17,
	SIUL2_PC_06_MSCR_S32_G1_WKUP18,
	SIUL2_PL_07_MSCR_S32_G1_WKUP19,
	SIUL2_PL_03_MSCR_S32_G1_WKUP20,
	SIUL2_PL_06_MSCR_S32_G1_WKUP21,
	SIUL2_PA_10_MSCR_S32_G1_WKUP22,
};

void wkpu_config_pinctrl(uint32_t wkup_irq)
{
	static const uint32_t wkpu_cfgs[] = {
		PCF_INIT(PCF_INPUT_ENABLE, 1),
	};
	const struct s32_pin_config wkpu_pinconf = {
		.pin = wkpu_gpio[wkup_irq],
		.function = SIUL2_MSCR_MUX_MODE_ALT0,
		.no_configs = ARRAY_SIZE(wkpu_cfgs),
		.configs = wkpu_cfgs,
	};
	const struct s32_peripheral_config wkpu_periph = {
		.no_configs = 1,
		.configs = &wkpu_pinconf,
	};

	s32_configure_peripheral_pinctrl(&wkpu_periph);
}

static const uint32_t i2c_cfgs[] = {
	PCF_INIT(PCF_OUTPUT_ENABLE, 1),
	PCF_INIT(PCF_INPUT_ENABLE, 1),
	PCF_INIT(PCF_DRIVE_OPEN_DRAIN, 1),
	PCF_INIT(PCF_SLEW_RATE, SIUL2_MSCR_S32_G1_SRC_133MHz),
};

static const struct s32_pin_config i2c0_pinconfs[] = {
	{
		.pin = SIUL2_MSCR_S32G_PB_00,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(i2c_cfgs),
		.configs = i2c_cfgs,
	},
	{
		.pin = SIUL2_PB_00_IMCR_S32G_I2C0_SDA,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	},
	{
		.pin = SIUL2_MSCR_S32G_PB_01,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(i2c_cfgs),
		.configs = i2c_cfgs,
	},
	{
		.pin = SIUL2_PB_01_IMCR_S32G_I2C0_SCLK,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	},
};

/* EEPROM - AT24C01B */

static const struct s32_peripheral_config i2c0_periph = {
	.no_configs = ARRAY_SIZE(i2c0_pinconfs),
	.configs = i2c0_pinconfs,
};

static const struct s32_pin_config i2c1_pinconfs[] = {
	{
		.pin = SIUL2_MSCR_S32G_PB_04,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(i2c_cfgs),
		.configs = i2c_cfgs,
	},
	{
		.pin = SIUL2_PB_04_IMCR_S32G_I2C1_SDA,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	},
	{
		.pin = SIUL2_MSCR_S32G_PB_03,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(i2c_cfgs),
		.configs = i2c_cfgs,
	},
	{
		.pin = SIUL2_PB_03_IMCR_S32G_I2C1_SCLK,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	},
};

/* Plaftorm board - PCI X16 Express (J99) */

static const struct s32_peripheral_config i2c1_periph = {
	.no_configs = ARRAY_SIZE(i2c1_pinconfs),
	.configs = i2c1_pinconfs,
};

static const struct s32_pin_config i2c4_pinconfs[] = {
	{
		.pin = SIUL2_MSCR_S32G_PC_01,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(i2c_cfgs),
		.configs = i2c_cfgs,
	},
	{
		.pin = SIUL2_PC_01_IMCR_S32G_I2C4_SDA,
		.function = SIUL2_MSCR_MUX_MODE_ALT3,
	},
	{
		.pin = SIUL2_MSCR_S32G_PC_02,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
		.no_configs = ARRAY_SIZE(i2c_cfgs),
		.configs = i2c_cfgs,
	},
	{
		.pin = SIUL2_PC_02_IMCR_S32G_I2C4_SCLK,
		.function = SIUL2_MSCR_MUX_MODE_ALT3,
	},
};

/* PMIC - I2C4 */

static const struct s32_peripheral_config i2c4_periph = {
	.no_configs = ARRAY_SIZE(i2c4_pinconfs),
	.configs = i2c4_pinconfs,
};

void i2c_config_pinctrl(void)
{
	s32_configure_peripheral_pinctrl(&i2c0_periph);
	s32_configure_peripheral_pinctrl(&i2c1_periph);
	s32_configure_peripheral_pinctrl(&i2c4_periph);
}
