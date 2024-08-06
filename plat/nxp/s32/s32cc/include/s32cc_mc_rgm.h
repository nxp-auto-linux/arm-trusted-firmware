/*
 * Copyright 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32CC___S32_MC_RGM_H__
#define S32CC___S32_MC_RGM_H__

#include <lib/utils_def.h>
#include <stdint.h>

#define S32_MC_RGM_BASE_ADDR		0x40078000ul
#define S32_MC_RGM_SIZE				0x1000ul
#define S32_MC_RGM_PRST_BASE_ADDR	(S32_MC_RGM_BASE_ADDR + 0x40)
#define S32_MC_RGM_PSTAT_BASE_ADDR	(S32_MC_RGM_BASE_ADDR + 0x140)
#define S32_MC_RGM_DRET_ADDR		(S32_MC_RGM_BASE_ADDR + 0x1C)
/* Peripheral reset */
#define S32_MC_RGM_PSTAT(p)		    (S32_MC_RGM_PSTAT_BASE_ADDR + 0x8 * p)
#define MC_RGM_PRST_PERIPH_N_RST(n)		BIT(n)
#define MC_RGM_STAT_PERIPH_N_STAT(n)	BIT(n)

#define PERIPH_3_RST				BIT(3)


/* Software-resettable domain/partition 0: M7 cores */
#define S32_MC_RGM_RST_DOMAIN_CM7	0

/* Software-resettable domain/partition 1: CA53 cores */
#define S32_MC_RGM_RST_DOMAIN_CA53	1
/* The entire domain defined by S32_MC_RGM_RST_DOMAIN_CA53 can be reset */
#define S32_MC_RGM_RST_CA53_PART_BIT	BIT(0)

#define MC_RGM_DES	(S32_MC_RGM_BASE_ADDR)
#define DES_F_POR	BIT(0)
#define DES_F_DR_ANY	0xc0073f5a

#define MC_RGM_FES	(S32_MC_RGM_BASE_ADDR + 0x8)
#define FES_F_EXR	BIT(0)
#define FES_F_FR_ANY	0xc0340058

enum reset_cause {
	CAUSE_POR,
	CAUSE_DESTRUCTIVE_RESET_DURING_RUN,
	CAUSE_DESTRUCTIVE_RESET_DURING_STANDBY,
	CAUSE_FUNCTIONAL_RESET_DURING_RUN,
	CAUSE_FUNCTIONAL_RESET_DURING_STANDBY,
	CAUSE_WAKEUP_DURING_STANDBY,
	CAUSE_ERROR,
	CAUSE_EXTERNAL_RESET_DURING_RUN,
	CAUSE_EXTERNAL_RESET_DURING_STANDBY,
	CAUSE_MAX_NUM
};

/* Reset Domain Controller definitions */
#define S32_RDC_BASE_ADDR	0x40080000ull
#define RDC_RD_CTRL(part)	(S32_RDC_BASE_ADDR + (part) * 4)
#define RDC_CTRL_UNLOCK		BIT(31)

uint32_t s32_mc_rgm_read(void *rgm, uint32_t part);
void s32_mc_rgm_periph_reset(void *rgm, uint32_t part, uint32_t value);
uint8_t get_rgm_a53_bit(uint8_t core);
uint8_t get_rgm_m7_bit(uint8_t core);

#endif /* S32CC___S32_MC_RGM_H__ */
