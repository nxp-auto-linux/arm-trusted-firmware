/*
 * Copyright 2019-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Pinmux of the peripherals whose pads are shared by the S32G2 and the
 * S32G3, but are not common to the whole S32G family.
 */
#include <common/debug.h>
#include <drivers/nxp/s32/i2c/s32_i2c.h>
#include <lib/utils_def.h>
#include "s32cc_bl_common.h"
#include "s32g_pinctrl.h"

/* LinFLEX 0 */
#define SIUL2_PC09_MSCR_S32_CC_UART0		(41)
#define SIUL2_PC10_MSCR_S32_CC_UART0		(42)
#define SIUL2_PC10_IMCR_S32_CC_UART0		(512)

static const uint32_t i2c_cfgs[] = {
	PCF_INIT(PCF_OUTPUT_ENABLE, 1),
	PCF_INIT(PCF_INPUT_ENABLE, 1),
	PCF_INIT(PCF_DRIVE_OPEN_DRAIN, 1),
	PCF_INIT(PCF_SLEW_RATE, SIUL2_MSCR_S32_G1_SRC_133MHz),
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

/* GPIO state for I2C4 bus-recovery (VR5510 PMIC bus). */
static const struct s32_pin_config i2c4_gpio_pinconfs[] = {
	{
		.pin = SIUL2_MSCR_S32G_PC_01,
		.function = SIUL2_MSCR_MUX_MODE_ALT0,
		.no_configs = ARRAY_SIZE(i2c_cfgs),
		.configs = i2c_cfgs,
	},
	{
		.pin = SIUL2_MSCR_S32G_PC_02,
		.function = SIUL2_MSCR_MUX_MODE_ALT0,
		.no_configs = ARRAY_SIZE(i2c_cfgs),
		.configs = i2c_cfgs,
	},
};

static const struct s32_peripheral_config i2c4_gpio_periph = {
	.no_configs = ARRAY_SIZE(i2c4_gpio_pinconfs),
	.configs = i2c4_gpio_pinconfs,
};

void i2c_config_pinctrl(void)
{
	int ret;

	ret = s32_configure_peripheral_pinctrl(&i2c4_periph);
	if (ret) {
		ERROR("Failed to configure peripheral I2C4 due to pinctrl error: %d\n",
			      ret);
		panic();
	}
}

/*
 * Attach the ERR050686 GPIO recovery descriptor to the PMIC bus, which is
 * the only bus the pinmux above brings out.
 */
void s32_i2c_apply_recovery(struct s32_i2c_bus *bus)
{
	struct s32_i2c_recovery rec;

	if (bus == NULL || bus->base != S32CC_I2C4_BASE)
		return;

	rec.scl_pin     = SIUL2_MSCR_S32G_PC_02;
	rec.sda_pin     = SIUL2_MSCR_S32G_PC_01;
	rec.gpio_cfg    = &i2c4_gpio_periph;
	rec.restore_cfg = &i2c4_periph;
	s32_i2c_set_recovery_info(bus, &rec);
}

/* LinFlexD-UART */
static const uint32_t uart_txd_cfgs[] = {
	PCF_INIT(PCF_SLEW_RATE, SIUL2_MSCR_S32_G1_SRC_133MHz),
	PCF_INIT(PCF_OUTPUT_ENABLE, 1),
};

static const uint32_t uart_rxd_cfgs[] = {
	PCF_INIT(PCF_SLEW_RATE, SIUL2_MSCR_S32_G1_SRC_133MHz),
	PCF_INIT(PCF_INPUT_ENABLE, 1),
};

static const struct s32_pin_config uart0_pinconfs[] = {
	{
		.pin = SIUL2_PC09_MSCR_S32_CC_UART0,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(uart_txd_cfgs),
		.configs = uart_txd_cfgs,
	},
	{
		.pin = SIUL2_PC10_MSCR_S32_CC_UART0,
		.function = SIUL2_MSCR_MUX_MODE_ALT0,
		.no_configs = ARRAY_SIZE(uart_rxd_cfgs),
		.configs = uart_rxd_cfgs,
	},
	{
		.pin = SIUL2_PC10_IMCR_S32_CC_UART0,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	}
};

static const struct s32_peripheral_config uart0_periph = {
	.no_configs = ARRAY_SIZE(uart0_pinconfs),
	.configs = uart0_pinconfs,
};

const struct s32_peripheral_config *s32g_get_uart0_pinctrl(void)
{
	return &uart0_periph;
}
