/*
 * Copyright 2021,2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/mmio.h>
#include <assert.h>
#include <s32cc_bl_common.h>
#include <s32cc_pinctrl.h>
#include <s32cc_scmi_pinctrl_utils.h>
#include <stdbool.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

struct siul2_mmap {
	unsigned long long base_pa;
	size_t size;
	unsigned int attr;
	bool mmap_added;
};

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

static inline int s32_mmap_siul2_regions(void)
{
	static struct siul2_mmap siul2_mmaps[] = {
		{
			.base_pa = SIUL2_0_BASE_ADDR,
			.size = MMU_ROUND_UP_TO_PAGE(SIUL2_0_SIZE),
			.attr = MT_DEVICE | MT_RW,
			.mmap_added = false,
		},
		{
			.base_pa = SIUL2_1_BASE_ADDR,
			.size = MMU_ROUND_UP_TO_PAGE(SIUL2_1_SIZE),
			.attr = MT_DEVICE | MT_RW,
			.mmap_added = false,
		},
	};
	unsigned int reg_idx;
	int ret;

	for (reg_idx = 0; reg_idx < ARRAY_SIZE(siul2_mmaps); reg_idx++) {
		struct siul2_mmap *reg = &siul2_mmaps[reg_idx];

		ret = s32_mmap_dynamic_region(reg->base_pa, reg->size, reg->attr, &reg->mmap_added);
		if (ret)
			return ret;
	}

	return 0;
}

uint32_t get_siul2_midr2_freq(void)
{
	int ret;

	ret = s32_mmap_siul2_regions();
	if (ret)
		panic();

	return ((mmio_read_32(SIUL2_MIDR2) & SIUL2_MIDR2_FREQ_MASK)
			>> SIUL2_MIDR2_FREQ_SHIFT);
}

int s32_get_pin_addr(uint16_t pin, uintptr_t *pin_addr)
{
	unsigned int i, temp;
	*pin_addr = 0;

	for (i = 0; i < ARRAY_SIZE(s32_pin_ranges); i++) {
		if (pin >= s32_pin_ranges[i].start &&
		    pin <= s32_pin_ranges[i].end) {
			temp = 4 * (pin - s32_pin_ranges[i].start);

			if (check_uptr_overflow(s32_pin_ranges[i].base, temp))
				return -EOVERFLOW;

			*pin_addr = s32_pin_ranges[i].base + temp;
			return 0;
		}
	}

	return -EINVAL;

}

static int
s32_configure_peripheral_pinctrl_scmi(const struct s32_peripheral_config *c)
{
	const struct s32_pin_config *cfg;
	unsigned int i;
	int ret = 0;

	for (i = 0; i < c->no_configs; i++) {
		cfg = &c->configs[i];

		if (cfg->no_configs > UINT32_MAX)
			return -EOVERFLOW;

		ret = s32_scmi_pinctrl_set_mux(&cfg->pin, &cfg->function, 1);
		if (ret)
			return ret;

		ret = s32_scmi_pinctrl_set_pcf(&cfg->pin, 1, cfg->configs,
					       cfg->no_configs);
		if (ret)
			return ret;
	}

	return 0;
}

int s32_configure_peripheral_pinctrl(const struct s32_peripheral_config *cfg)
{
	unsigned int i;
	uintptr_t addr;
	int ret;

	if (is_pinctrl_over_scmi_used()) {
		return s32_configure_peripheral_pinctrl_scmi(cfg);
	}

	ret = s32_mmap_siul2_regions();
	if (ret)
		return ret;

	for (i = 0; i < cfg->no_configs; i++) {
		ret = s32_get_pin_addr(cfg->configs[i].pin, &addr);
		if (ret)
			return ret;

		mmio_write_32(addr, get_mscr_config(&cfg->configs[i]));
	}

	return 0;
}

int s32_plat_config_uart_pinctrl(void)
{
	int ret;

	ret = s32_config_linflex_pinctrl(S32_LINFLEX_MODULE);
	if (ret)
		ERROR("Failed to configure UART pinctrl with error: %d\n",
			      ret);

	return ret;
}

int s32_plat_config_sdhc_pinctrl(void)
{
	int ret;

	ret = s32_configure_peripheral_pinctrl(&sdhc_periph);
	if (ret)
		ERROR("Failed to configure SDHC pinctrl with error: %d\n",
			      ret);

	return ret;
}
