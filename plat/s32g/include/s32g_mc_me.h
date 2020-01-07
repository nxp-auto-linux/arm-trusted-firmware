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
#define S32G_MC_ME_CTL_KEY		(S32G_MC_ME_BASE_ADDR)
#define S32G_MC_ME_CTL_KEY_KEY		0x00005AF0
#define S32G_MC_ME_CTL_KEY_INVERTEDKEY	0x0000A50F

/* PRTNn registers */
#define S32G_MC_ME_PRTN_N_BASE(n) \
	(S32G_MC_ME_BASE_ADDR + 0x100 + (n) * 0x200)
#define S32G_MC_ME_PRTN_N_PCONF(n) \
	(S32G_MC_ME_PRTN_N_BASE(n) + 0x0)
#define S32G_MC_ME_PRTN_N_PUPD(n) \
	(S32G_MC_ME_PRTN_N_BASE(n) + 0x4)
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

/* PRTNn_COFBm registers */

#define S32G_MC_ME_PRTN_N_COFB_0_CLKEN(n) \
	(S32G_MC_ME_PRTN_N_BASE(n) + 0x30)


#define S32G_MC_ME_CA53_PART	1
#define S32G_MC_ME_DDR_0_PART	0
#define S32G_MC_ME_USDHC_PART	0
#define S32G_MC_ME_DDR_0_REQ	1
#define S32G_MC_ME_USDHC_REQ	0


void s32g_kick_secondary_ca53_cores(void);
void mc_me_enable_partition_block(uint32_t part, uint32_t block);


#endif /* __S32G_MC_ME_H__ */
