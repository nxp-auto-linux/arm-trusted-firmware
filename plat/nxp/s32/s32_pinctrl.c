/*
 * Copyright 2021,2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/mmio.h>
#include <assert.h>
#include <s32_bl_common.h>
#include <s32_pinctrl.h>
#include <s32_scmi_pinctrl.h>

/*
 * Pinctrl for LinFlexD-UART
 */

/* LinFLEX 0 */
#define SIUL2_PC09_MSCR_S32_G1_UART0		(41)
#define SIUL2_PC10_MSCR_S32_G1_UART0		(42)
#define SIUL2_PC10_IMCR_S32_G1_UART0		(512)

static const struct s32_pin_range s32_pin_ranges[] = {
	{
		.start = SIUL2_0_MSCR_START,
		.end = SIUL2_0_MSCR_END,
		.base = SIUL2_0_MSCR_BASE,
	},
	{
		.start = SIUL2_1_MSCR_START,
		.end = SIUL2_1_MSCR_END,
		.base = SIUL2_1_MSCR_BASE,
	},
	{
		.start = SIUL2_0_IMCR_START,
		.end = SIUL2_0_IMCR_END,
		.base = SIUL2_0_IMCR_BASE,
	},
	{
		.start = SIUL2_1_IMCR_START,
		.end = SIUL2_1_IMCR_END,
		.base = SIUL2_1_IMCR_BASE,
	},
};

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
		.pin = SIUL2_PC09_MSCR_S32_G1_UART0,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(uart_txd_cfgs),
		.configs = uart_txd_cfgs,
	},
	{
		.pin = SIUL2_PC10_MSCR_S32_G1_UART0,
		.function = SIUL2_MSCR_MUX_MODE_ALT0,
		.no_configs = ARRAY_SIZE(uart_rxd_cfgs),
		.configs = uart_rxd_cfgs,
	},
	{
		.pin = SIUL2_PC10_IMCR_S32_G1_UART0,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	}
};

static const struct s32_peripheral_config uart0_periph = {
	.no_configs = ARRAY_SIZE(uart0_pinconfs),
	.configs = uart0_pinconfs,
};

/* LinFLEX 1 */
#define SIUL2_PB09_MSCR_S32_G1_UART1		(25)
#define SIUL2_PB10_MSCR_S32_G1_UART1		(26)
#define SIUL2_PB10_IMCR_S32_G1_UART1		(736)

static const struct s32_pin_config uart1_pinconfs[] = {
	{
		.pin = SIUL2_PB09_MSCR_S32_G1_UART1,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(uart_txd_cfgs),
		.configs = uart_txd_cfgs,
	},
	{
		.pin = SIUL2_PB10_MSCR_S32_G1_UART1,
		.function = SIUL2_MSCR_MUX_MODE_ALT0,
		.no_configs = ARRAY_SIZE(uart_rxd_cfgs),
		.configs = uart_rxd_cfgs,
	},
	{
		.pin = SIUL2_PB10_IMCR_S32_G1_UART1,
		.function = SIUL2_MSCR_MUX_MODE_ALT3,
	}
};

static const struct s32_peripheral_config uart1_periph = {
	.no_configs = ARRAY_SIZE(uart1_pinconfs),
	.configs = uart1_pinconfs,
};

static const uint32_t usdhc_base_cfgs[] = {
	PCF_INIT(PCF_SLEW_RATE, SIUL2_MSCR_S32_G1_SRC_150MHz),
	PCF_INIT(PCF_OUTPUT_ENABLE, 1),
	PCF_INIT(PCF_INPUT_ENABLE, 1),
	PCF_INIT(PCF_BIAS_PULL_DOWN, 1),
};

static const uint32_t usdhc_pull_up_cfgs[] = {
	PCF_INIT(PCF_SLEW_RATE, SIUL2_MSCR_S32_G1_SRC_150MHz),
	PCF_INIT(PCF_OUTPUT_ENABLE, 1),
	PCF_INIT(PCF_INPUT_ENABLE, 1),
	PCF_INIT(PCF_BIAS_PULL_UP, 1),
};

static const uint32_t usdhc_rst_cfgs[] = {
	PCF_INIT(PCF_SLEW_RATE, SIUL2_MSCR_S32_G1_SRC_150MHz),
	PCF_INIT(PCF_OUTPUT_ENABLE, 1),
	PCF_INIT(PCF_BIAS_PULL_DOWN, 1),
};

static const struct s32_pin_config sdhc_pinconfs[] = {
	{
		.pin = 46,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(usdhc_base_cfgs),
		.configs = usdhc_base_cfgs,
	},
	{
		.pin = 47,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(usdhc_pull_up_cfgs),
		.configs = usdhc_pull_up_cfgs,
	},
	{
		.pin = 515,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	},
	{
		.pin = 48,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(usdhc_pull_up_cfgs),
		.configs = usdhc_pull_up_cfgs,
	},
	{
		.pin = 516,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	},
	{
		.pin = 49,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(usdhc_pull_up_cfgs),
		.configs = usdhc_pull_up_cfgs,
	},
	{
		.pin = 517,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	},
	{
		.pin = 50,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(usdhc_pull_up_cfgs),
		.configs = usdhc_pull_up_cfgs,
	},
	{
		.pin = 520,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	},
	{
		.pin = 51,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(usdhc_pull_up_cfgs),
		.configs = usdhc_pull_up_cfgs,
	},
	{
		.pin = 521,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	},
	{
		.pin = 52,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(usdhc_pull_up_cfgs),
		.configs = usdhc_pull_up_cfgs,
	},
	{
		.pin = 522,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	},
	{
		.pin = 53,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(usdhc_pull_up_cfgs),
		.configs = usdhc_pull_up_cfgs,
	},
	{
		.pin = 523,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	},
	{
		.pin = 54,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(usdhc_pull_up_cfgs),
		.configs = usdhc_pull_up_cfgs,
	},
	{
		.pin = 519,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	},
	{
		.pin = 55,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(usdhc_pull_up_cfgs),
		.configs = usdhc_pull_up_cfgs,
	},
	{
		.pin = 518,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	},
	{
		.pin = 56,
		.function = SIUL2_MSCR_MUX_MODE_ALT1,
		.no_configs = ARRAY_SIZE(usdhc_rst_cfgs),
		.configs = usdhc_rst_cfgs,
	},
	{
		.pin = 524,
		.function = SIUL2_MSCR_MUX_MODE_ALT2,
	},
};

static const struct s32_peripheral_config sdhc_periph = {
	.no_configs = ARRAY_SIZE(sdhc_pinconfs),
	.configs = sdhc_pinconfs,
};

static void linflex_config_pinctrl(int lf)
{
	if (!lf)
		s32_configure_peripheral_pinctrl(&uart0_periph);
	else
		s32_configure_peripheral_pinctrl(&uart1_periph);
}

static uint32_t convert_config(uint32_t config)
{
	enum pin_conf cfg = (enum pin_conf)PCF_GET_CFG(config);
	uint32_t val = PCF_GET_VAL(config);
	uint32_t mscr = 0;

	switch (cfg) {
	/* All pins are persistent over suspend */
	case PCF_DRIVE_OPEN_DRAIN:
		mscr |= SIUL2_MSCR_ODE;
		break;
	case PCF_OUTPUT_ENABLE:
		if (val)
			mscr |= SIUL2_MSCR_OBE;
		break;
	case PCF_INPUT_ENABLE:
		if (val)
			mscr |= SIUL2_MSCR_IBE;
		break;
	case PCF_SLEW_RATE:
		mscr |= SIUL2_MSCR_SRE_MASK(val);
		break;
	case PCF_BIAS_PULL_UP:
		if (val)
			mscr |= SIUL2_MSCR_PUS | SIUL2_MSCR_PUE;
		break;
	case PCF_BIAS_PULL_DOWN:
		if (val)
			mscr |= SIUL2_MSCR_PUE;
		break;
	default:
		break;
	}

	return mscr;
}

static uint32_t get_mscr_config(const struct s32_pin_config *cfg)
{
	unsigned int i;
	uint32_t mscr;

	mscr = cfg->function;
	for (i = 0; i < cfg->no_configs; i++)
		mscr |= convert_config(cfg->configs[i]);

	return mscr;
}

uintptr_t s32_get_pin_addr(uint16_t pin)
{
	unsigned int i, temp;

	for (i = 0; i < ARRAY_SIZE(s32_pin_ranges); i++) {
		if (pin >= s32_pin_ranges[i].start &&
		    pin <= s32_pin_ranges[i].end) {
			temp = 4 * (pin - s32_pin_ranges[i].start);

			if (check_uptr_overflow(s32_pin_ranges[i].base, temp))
				panic();

			return s32_pin_ranges[i].base + temp;
		}
	}

	return 0;

}

static void
s32_configure_peripheral_pinctrl_scmi(const struct s32_peripheral_config *c)
{
	const struct s32_pin_config *cfg;
	unsigned int i;
	int ret = 0;

	for (i = 0; i < c->no_configs; i++) {
		cfg = &c->configs[i];

		if (cfg->no_configs > UINT32_MAX)
			panic();

		ret = s32_scmi_pinctrl_set_mux(&cfg->pin, &cfg->function, 1);
		if (ret)
			panic();

		ret = s32_scmi_pinctrl_set_pcf(&cfg->pin, 1, cfg->configs,
					       cfg->no_configs);
		if (ret)
			panic();
	}
}

void s32_configure_peripheral_pinctrl(const struct s32_peripheral_config *cfg)
{
	unsigned int i;
	uintptr_t addr;

	if (is_pinctrl_over_scmi_used()) {
		s32_configure_peripheral_pinctrl_scmi(cfg);
		return;
	}

	for (i = 0; i < cfg->no_configs; i++) {
		addr = s32_get_pin_addr(cfg->configs[i].pin);
		if (!addr)
			panic();

		mmio_write_32(addr, get_mscr_config(&cfg->configs[i]));
	}
}

void s32_plat_config_uart_pinctrl(void)
{
	linflex_config_pinctrl(S32_LINFLEX_MODULE);
}

void s32_plat_config_sdhc_pinctrl(void)
{
	s32_configure_peripheral_pinctrl(&sdhc_periph);
}
