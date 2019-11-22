/*
 * Magic Carpet (MC) modules definitions for S32G274A and compatible SoCs
 *
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __S32G_MC_ME_H__
#define __S32G_MC_ME_H__

#include "platform_def.h"

/*
 * MC Mode Entry
 */

#define S32G_MC_ME_BASE_ADDR	0x40088000ul
#define S32G_MC_ME_SIZE		0x1000ul

#define S32G_MC_ME_PRIMARY_CORE_MASK	((1ul << S32G_PLAT_PRIMARY_CPU) & 0xF)
#define S32G_MC_ME_SECONDARY_CORE_MASK	(~S32G_MC_ME_PRIMARY_CORE_MASK & 0xF)

/* CTL_KEY register */
#define S32G_MC_ME_CTL_KEY		S32G_MC_ME_BASE_ADDR
#define S32G_MC_ME_CTL_KEY_KEY		0x00005AF0
#define S32G_MC_ME_CTL_KEY_INVERTEDKEY	0x0000A50F

/* PRTNn registers */
#define S32G_MC_ME_PRTN_N_BASE(n) \
	(S32G_MC_ME_BASE_ADDR + 0x100 + (n) * 0x200)
#define S32G_MC_ME_PRTN_N_PCONF(n) \
	(S32G_MC_ME_PRTN_N_BASE(n) + 0x4)
#define S32G_MC_ME_PRTN_N_PUPD(n) \
	(S32G_MC_ME_PRTN_N_BASE(n) + 0x8)
/* PRTNn_PCONF[PCE] mask */
#define S32G_MC_ME_PRTN_N_PCONF_PCE_MASK	0x1ul
/* PRTNn_PUPD[PCUD] mask */
#define S32G_MC_ME_PRTN_N_PUPD_PCUD_MASK	0x1ul

/* PRTNn_COREm registers */
#define S32G_MC_ME_PRTN_N_CORE_M_BASE(n, m) \
	(S32G_MC_ME_BASE_ADDR + 0x140 + (n) * 0x200 + (m) * 0x20)
#define S32G_MC_ME_PRTN_N_CORE_M_ADDR(n, m) \
	(S32G_MC_ME_PRTN_N_CORE_M_BASE(n, m) + 0xc)
#define S32G_MC_ME_PRTN_N_CORE_M_PCONF(n, m) \
	S32G_MC_ME_PRTN_N_CORE_M_BASE(n, m)
#define S32G_MC_ME_PRTN_N_CORE_M_PUPD(n, m) \
	(S32G_MC_ME_PRTN_N_CORE_M_BASE(n, m) + 0x4)
#define S32G_MC_ME_PRTN_N_CORE_M_STAT(n, m) \
	(S32G_MC_ME_PRTN_N_CORE_M_BASE(n, m) + 0x8)
/* PRTNn_COREm_PCONF[CCE] mask */
#define S32G_MC_ME_PRTN_N_CORE_M_PCONF_CCE_MASK		0x1ul
/* PRTNn_COREm_PUPD[CCUPD] mask */
#define S32G_MC_ME_PRTN_N_CORE_M_PUPD_CCUPD_MASK	0x1ul
/* PRTNn_COREm_STAT[CCS] mask */
#define S32G_MC_ME_PRTN_N_CORE_M_STAT_CCS_MASK		0x1ul

#define S32G_MC_ME_CA53_PART	1

/*
 * MC Reset Generation Module
 */
#define S32G_MC_RGM_BASE_ADDR	0x40078000ul
#define S32G_MC_RGM_SIZE	0x1000ul
#define S32G_MC_RGM_PRST_BASE_ADDR	(S32G_MC_RGM_BASE_ADDR + 0x40)
#define S32G_MC_RGM_PSTAT_BASE_ADDR	(S32G_MC_RGM_BASE_ADDR + 0x140)
/* Peripheral reset */
#define S32G_MC_RGM_PRST(p)	(S32G_MC_RGM_PRST_BASE_ADDR + 0x8 * p)
#define S32G_MC_RGM_PSTAT(p)	(S32G_MC_RGM_PSTAT_BASE_ADDR + 0x8 * p)

/* Software-resettable domain/partition 1: CA53 cores */
#define S32G_MC_RGM_RST_DOMAIN_CA53	1
/* Bit corresponding to CA53_n in the cores' RGM reset partition (n=0..3) */
#define S32G_MC_RGM_RST_CA53_BIT(n)	BIT(n + 1)
/* The entire domain defined by S32G_MC_RGM_RST_DOMAIN_CA53 can be reset */
#define S32G_MC_RGM_RST_CA53_PART_BIT	BIT(0)

void s32g_kick_secondary_ca53_cores(void);


#endif /* __S32G_MC_ME_H__ */
