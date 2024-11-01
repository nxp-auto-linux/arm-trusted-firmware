/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/mmio.h>
#include <assert.h>
#include "s32cc_pinctrl.h"

/*
 * Pinctrl for LinFlexD-UART
 */

/* LinFLEX 0 */
#define SIUL2_PC09_MSCR_S32_CC_UART0		(41)
#define SIUL2_PC10_MSCR_S32_CC_UART0		(42)
#define SIUL2_PC10_IMCR_S32_CC_UART0		(512)

/* LinFLEX 1 */
#define SIUL2_PA11_MSCR_S32_R45_UART1		(11)
#define SIUL2_PA12_MSCR_S32_R45_UART1		(12)
#define SIUL2_PA11_IMCR_S32_R45_UART1		(674)

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

static const struct s32_pin_config uart1_pinconfs[] = {
	{
		.pin = SIUL2_PA12_MSCR_S32_R45_UART1,
		.function = SIUL2_MSCR_MUX_MODE_ALT3,
		.no_configs = ARRAY_SIZE(uart_txd_cfgs),
		.configs = uart_txd_cfgs,
	},
	{
		.pin = SIUL2_PA11_MSCR_S32_R45_UART1,
		.function = SIUL2_MSCR_MUX_MODE_ALT0,
		.no_configs = ARRAY_SIZE(uart_rxd_cfgs),
		.configs = uart_rxd_cfgs,
	},
	{
		.pin = SIUL2_PA11_IMCR_S32_R45_UART1,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	}
};

static const struct s32_peripheral_config uart1_periph = {
	.no_configs = ARRAY_SIZE(uart1_pinconfs),
	.configs = uart1_pinconfs,
};

int s32_config_linflex_pinctrl(int lf)
{
	if (!lf)
		return s32_configure_peripheral_pinctrl(&uart0_periph);
	else
		return s32_configure_peripheral_pinctrl(&uart1_periph);
}
