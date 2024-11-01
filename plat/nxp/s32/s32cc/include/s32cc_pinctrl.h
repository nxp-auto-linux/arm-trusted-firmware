/*
 * Copyright 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32CC__S32_PINCTRL_H_
#define S32CC__S32_PINCTRL_H_

#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <stddef.h>
#include "platform_def.h"

#define SIUL2_MIDR1		(SIUL2_0_BASE_ADDR + 0x00000004)
#define SIUL2_MIDR2		(SIUL2_0_BASE_ADDR + 0x00000008)
#define SIUL2_0_MSCR_BASE	(SIUL2_0_BASE_ADDR + 0x00000240)
#define SIUL2_0_IMCR_BASE	(SIUL2_0_BASE_ADDR + 0x00000A40)
/* redefine these for PLAT_s32r if SIUL2_1 is needed*/
#define SIUL2_1_MSCR_BASE	(SIUL2_1_BASE_ADDR + 0x00000400)
#define SIUL2_1_IMCR_BASE	(SIUL2_1_BASE_ADDR + 0x00000C1C)

#define SIUL2_MSCR_MUX_MODE_ALT0 (0)
#define SIUL2_MSCR_MUX_MODE_ALT1 (1)
#define SIUL2_MSCR_MUX_MODE_ALT2 (2)
#define SIUL2_MSCR_MUX_MODE_ALT3 (3)
#define SIUL2_MSCR_MUX_MODE_ALT4 (4)
#define SIUL2_MSCR_MUX_MODE_ALT5 (5)

#define SIUL2_MSCR_OBE	BIT_32(21)
#define SIUL2_MSCR_ODE	BIT_32(20)
#define SIUL2_MSCR_IBE	BIT_32(19)
#define SIUL2_MSCR_PUS	BIT_32(12)
#define SIUL2_MSCR_PUE	BIT_32(13)

#define SIUL2_MSCR_S32_G1_SRC_133MHz	(6)
#define SIUL2_MSCR_S32_G1_SRC_150MHz	(5)

#define SIUL2_MSCR_SRE_MASK(X)	(((X) & GENMASK(3, 0)) << 14)

#define PCF_INIT(c, v) ((v) << 8 | ((c) & 0xFF))
#define PCF_GET_CFG(c) ((c) & 0xFF)
#define PCF_GET_VAL(c) ((c) >> 8)

enum pin_conf {
	PCF_BIAS_PULL_DOWN = 3,
	PCF_BIAS_PULL_UP = 5,
	PCF_DRIVE_OPEN_DRAIN = 6,
	PCF_INPUT_ENABLE = 12,
	PCF_OUTPUT_ENABLE = 18,
	PCF_SLEW_RATE = 23,
	PCF_MAX,
};

struct s32_pin_range {
	uint16_t start;
	uint16_t end;
	uintptr_t base;
};

struct s32_pin_config {
	uint16_t pin;
	uint16_t function;
	size_t no_configs;
	const uint32_t *configs;
};

struct s32_peripheral_config {
	size_t no_configs;
	const struct s32_pin_config *configs;
};

/* SIUL2_MIDR2 masks */
#define SIUL2_MIDR2_FREQ_SHIFT		(16)
#define SIUL2_MIDR2_FREQ_MASK		(0xF << SIUL2_MIDR2_FREQ_SHIFT)

int s32_get_pin_addr(uint16_t pin, uintptr_t *pin_addr);

int s32_config_linflex_pinctrl(int lf);
int s32_configure_peripheral_pinctrl(const struct s32_peripheral_config *cfg);
int s32_plat_config_uart_pinctrl(void);
int s32_plat_config_sdhc_pinctrl(void);

uint32_t get_siul2_midr2_freq(void);

#endif /* S32CC__S32_PINCTRL_H_ */

