/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __S32G_MC_RGM_H__
#define __S32G_MC_RGM_H__

#define S32G_MC_RGM_BASE_ADDR	0x40078000ul
#define S32G_MC_RGM_SIZE	0x1000ul
#define S32G_MC_RGM_PRST_BASE_ADDR	(S32G_MC_RGM_BASE_ADDR + 0x40)
#define S32G_MC_RGM_PSTAT_BASE_ADDR	(S32G_MC_RGM_BASE_ADDR + 0x140)
/* Peripheral reset */
#define S32G_MC_RGM_PRST(p)	(S32G_MC_RGM_PRST_BASE_ADDR + 0x8 * p)
#define S32G_MC_RGM_PSTAT(p)	(S32G_MC_RGM_PSTAT_BASE_ADDR + 0x8 * p)
#define MC_RGM_PRST_PERIPH_N_RST(n)	BIT(n)
#define MC_RGM_STAT_PERIPH_N_STAT(n)	BIT(n)

#define PERIPH_3_RST		BIT(3)

/* Software-resettable domain/partition 1: CA53 cores */
#define S32G_MC_RGM_RST_DOMAIN_CA53	1
/* Bit corresponding to CA53_n in the cores' RGM reset partition (n=0..3) */
#define S32G_MC_RGM_RST_CA53_BIT(n)	BIT(n + 1)
/* The entire domain defined by S32G_MC_RGM_RST_DOMAIN_CA53 can be reset */
#define S32G_MC_RGM_RST_CA53_PART_BIT	BIT(0)

#define MC_RGM_DES	(S32G_MC_RGM_BASE_ADDR)
#define DES_F_POR	BIT(0)
#define DES_F_DR_ANY	0xc0073f5a

#define MC_RGM_FES	(S32G_MC_RGM_BASE_ADDR + 0x8)
#define FES_F_FR_ANY	0xc0340058

#define MC_RGM_RDSS	(S32G_MC_RGM_BASE_ADDR + 0x24)
#define RDSS_FES_RES	BIT(1)
#define RDSS_DES_RES	BIT(0)

enum reset_cause {
	CAUSE_POR,
	CAUSE_DESTRUCTIVE_RESET_DURING_RUN,
	CAUSE_DESTRUCTIVE_RESET_DURING_STANDBY,
	CAUSE_FUNCTIONAL_RESET_DURING_RUN,
	CAUSE_FUNCTIONAL_RESET_DURING_STANDBY,
	CAUSE_WAKEUP_DURING_STANDBY,
	CAUSE_ERROR
};

/* Reset Domain Controller definitions */
#define S32G_RDC_BASE_ADDR	0x40080000ull
#define RDC_RD_CTRL(part)	(S32G_RDC_BASE_ADDR + (part) * 4)
#define RDC_CTRL_UNLOCK		BIT(31)
#define RDC_CTRL_XBAR_DISABLE	BIT(3)

#endif /* __S32G_MC_RGM_H__ */
