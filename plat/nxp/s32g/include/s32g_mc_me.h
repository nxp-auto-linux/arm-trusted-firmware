/*
 * MC Mode Entry definitions for S32G274A and compatible SoCs
 *
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __S32G_MC_ME_H__
#define __S32G_MC_ME_H__

#include "platform_def.h"


#define S32G_MC_ME_BASE_ADDR	0x40088000ul
#define S32G_MC_ME_SIZE		0x1000ul

#define MC_ME_MODE_CONF		(S32G_MC_ME_BASE_ADDR + 0x4)
#define MC_ME_MODE_CONF_DRST	BIT(0)
#define MC_ME_MODE_CONF_FRST	BIT(1)
#define MC_ME_MODE_CONF_STANDBY	BIT(15)

#define MC_ME_MODE_UPD		(S32G_MC_ME_BASE_ADDR + 0x8)
#define MC_ME_MODE_UPD_UPD	BIT(0)

#define MC_ME_MODE_STAT		(S32G_MC_ME_BASE_ADDR + 0xc)
#define MODE_STAT_PREV_MODE	BIT(0)

#define MC_ME_MAIN_COREID	(S32G_MC_ME_BASE_ADDR + 0x10)
#define MC_ME_COREID_PIDX(n)	((n) << 8)
#define MC_ME_COREID_CIDX(n)	((n) << 0)

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
#define S32G_MC_ME_PRTN_N_STAT(n) \
	(S32G_MC_ME_PRTN_N_BASE(n) + 0x8)

/* COFB0 */
#define S32G_MC_ME_PRTN_N_COFB0_STAT(n) \
	(S32G_MC_ME_PRTN_N_BASE(n) + 0x10)
#define S32G_MC_ME_PRTN_N_COFB0_CLKEN(n) \
	(S32G_MC_ME_PRTN_N_BASE(n) + 0x30)

#define S32G_MC_ME_PRTN_N_PCONF_PCE_MASK	BIT(0)
#define S32G_MC_ME_PRTN_N_PCONF_OSSE_MASK	BIT(2)
#define S32G_MC_ME_PRTN_N_PUPD_PCUD_MASK	BIT(0)
#define S32G_MC_ME_PRTN_N_PUPD_OSSUD_MASK	BIT(2)
#define S32G_MC_ME_PRTN_N_REQ(n)		BIT(n)

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
#define S32G_MC_ME_PRTN_N_CORE_M_PCONF_CCE_MASK		0x1ul
#define S32G_MC_ME_PRTN_N_CORE_M_PUPD_CCUPD_MASK	0x1ul
#define S32G_MC_ME_PRTN_N_CORE_M_STAT_CCS_MASK		BIT(0)
#define S32G_MC_ME_PRTN_N_CORE_M_STAT_WFI_MASK		BIT(31)

/* PRTNn_COFBm registers */
#define S32G_MC_ME_PRTN_N_COFB_0_CLKEN(n) \
	(S32G_MC_ME_PRTN_N_BASE(n) + 0x30)

enum s32g_mc_me_part_no {
	S32G_MC_ME_PRTN0 = 0,
	S32G_MC_ME_PRTN1,
	S32G_MC_ME_PRTN2,
	S32G_MC_ME_PRTN3,
};
#define S32G_MC_ME_CA53_PART	S32G_MC_ME_PRTN1
#define S32G_MC_ME_CM7_PART	S32G_MC_ME_PRTN0
#define S32G_MC_ME_DDR_0_PART	S32G_MC_ME_PRTN0
#define S32G_MC_ME_USDHC_PART	S32G_MC_ME_PRTN0
#define S32G_MC_ME_PFE_PART	S32G_MC_ME_PRTN2

#define S32G_MC_ME_DDR_0_REQ	1
#define S32G_MC_ME_USDHC_REQ	0

/* Standby master core: A53, cluster 0, core 0*/
#define S32G_STBY_MASTER_CORE	0
#define S32G_STBY_MASTER_PART	1

bool s32g_core_in_reset(uint32_t core);
void s32g_kick_secondary_ca53_core(uint32_t core, uintptr_t entrypoint);
void s32g_turn_off_core(uint8_t part, uint8_t core);
void s32g_reset_core(uint8_t part, uint8_t core);
void s32g_disable_cofb_clk(uint8_t part, uint32_t keep_blocks);
void s32g_set_stby_master_core(uint8_t part, uint8_t core);
void mc_me_enable_partition_block(uint32_t part, uint32_t block);
void mc_me_enable_partition(uint32_t part);
void s32g_destructive_reset(void);


#endif /* __S32G_MC_ME_H__ */
