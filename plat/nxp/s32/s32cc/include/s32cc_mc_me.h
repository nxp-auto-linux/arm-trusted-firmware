/*
 * MC Mode Entry definitions for S32GEN1 and compatible SoCs
 *
 * Copyright 2021-2022, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32CC___S32_MC_ME_H__
#define S32CC___S32_MC_ME_H__

#include "platform_def.h"
#include <stdbool.h>
#include <stddef.h>

#define S32_MC_ME_BASE_ADDR	0x40088000ul
#define S32_MC_ME_SIZE		0x1000ul

#define MC_ME_MODE_CONF		(S32_MC_ME_BASE_ADDR + 0x4)
#define MC_ME_MODE_CONF_DRST	BIT(0)
#define MC_ME_MODE_CONF_FRST	BIT(1)

#define MC_ME_MODE_UPD		(S32_MC_ME_BASE_ADDR + 0x8)
#define MC_ME_MODE_UPD_UPD	BIT(0)

/* CTL_KEY register */
#define S32_MC_ME_CTL_KEY  (S32_MC_ME_BASE_ADDR)
#define S32_MC_ME_CTL_KEY_KEY		    0x00005AF0
#define S32_MC_ME_CTL_KEY_INVERTEDKEY	0x0000A50F

/* PRTNn registers */
#define S32_MC_ME_PRTN_N_PCONF_OFF	0x0
#define S32_MC_ME_PRTN_N_PUPD_OFF	0x4
#define S32_MC_ME_PRTN_N_STAT_OFF	0x8

#define S32_MC_ME_PRTN_N_BASE(n) \
	(S32_MC_ME_BASE_ADDR + 0x100ul + (n) * 0x200ul)
#define S32_MC_ME_PRTN_N_PCONF(n) \
	(S32_MC_ME_PRTN_N_BASE(n) + S32_MC_ME_PRTN_N_PCONF_OFF)
#define S32_MC_ME_PRTN_N_PUPD(n) \
	(S32_MC_ME_PRTN_N_BASE(n) + S32_MC_ME_PRTN_N_PUPD_OFF)
#define S32_MC_ME_PRTN_N_STAT(n) \
	(S32_MC_ME_PRTN_N_BASE(n) + S32_MC_ME_PRTN_N_STAT_OFF)

/* COFB0 */
#define S32_MC_ME_PRTN_N_COFB0_STAT(n) \
	(S32_MC_ME_PRTN_N_BASE(n) + 0x10)
#define S32_MC_ME_PRTN_N_COFB0_CLKEN(n) \
	(S32_MC_ME_PRTN_N_BASE(n) + 0x30)

#define S32_MC_ME_PRTN_N_PCONF_PCE_MASK     BIT(0)
#define S32_MC_ME_PRTN_N_PCONF_OSSE_MASK	BIT(2)
#define S32_MC_ME_PRTN_N_PUPD_PCUD_MASK     BIT(0)
#define S32_MC_ME_PRTN_N_PUPD_OSSUD_MASK	BIT(2)
#define S32_MC_ME_PRTN_N_REQ(n)             BIT(n)

/* PRTNn_COREm registers */
#define MC_ME_PRTN_PART(n, m) \
	(S32_MC_ME_BASE_ADDR + 0x140ul + (n) * 0x200ul + \
	 (m) * 0x20ul)
#define S32_MC_ME_PRTN_N_CORE_M_BASE(n, m) \
	MC_ME_PRTN_PART(n, mc_me_core2prtn_core_id((n), (m)))
#define S32_MC_ME_PRTN_N_CORE_M_ADDR(n, m) \
	(S32_MC_ME_PRTN_N_CORE_M_BASE(n, m) + 0xc)
#define S32_MC_ME_PRTN_N_CORE_M_PCONF(n, m) \
	(S32_MC_ME_PRTN_N_CORE_M_BASE(n, m) + S32_MC_ME_PRTN_N_PCONF_OFF)
#define S32_MC_ME_PRTN_N_CORE_M_PUPD(n, m) \
	(S32_MC_ME_PRTN_N_CORE_M_BASE(n, m) + S32_MC_ME_PRTN_N_PUPD_OFF)
#define S32_MC_ME_PRTN_N_CORE_M_STAT(n, m) \
	(S32_MC_ME_PRTN_N_CORE_M_BASE(n, m) + S32_MC_ME_PRTN_N_STAT_OFF)
#define S32_MC_ME_PRTN_N_CORE_M_PCONF_CCE_MASK		0x1ul
#define S32_MC_ME_PRTN_N_CORE_M_PUPD_CCUPD_MASK	0x1ul
#define S32_MC_ME_PRTN_N_CORE_M_STAT_CCS_MASK		BIT(0)
#define S32_MC_ME_PRTN_N_CORE_M_STAT_WFI_MASK		BIT(31)

/* PRTNn_COFBm registers */
#define S32_MC_ME_PRTN_N_COFB_0_CLKEN(n) \
	(S32_MC_ME_PRTN_N_BASE(n) + 0x30)

enum s32_mc_me_part_no {
	S32_MC_ME_PRTN0 = 0,
	S32_MC_ME_PRTN1,
	S32_MC_ME_PRTN2,
	S32_MC_ME_PRTN3,
};
#define S32_MC_ME_CA53_PART     S32_MC_ME_PRTN1
#define S32_MC_ME_CM7_PART		S32_MC_ME_PRTN0
#define S32_MC_ME_DDR_0_PART	S32_MC_ME_PRTN0
#define S32_MC_ME_USDHC_PART	S32_MC_ME_PRTN0

#define S32_MC_ME_DDR_0_REQ	1
#define S32_MC_ME_USDHC_REQ	0

struct a53_haddr_mapping {
	uint32_t reg; /** GPR register offset */
	uint32_t field_off; /** Field offset */
};

void mc_me_apply_hw_changes(void);

bool is_a53_core_in_reset(uint32_t core);
void s32_set_core_entrypoint(uint32_t core, uint64_t entrypoint);
void s32_kick_secondary_ca53_core(uint32_t core);
void s32_turn_off_core(uint8_t part, uint8_t core);
void s32_turn_off_mcores(uint32_t skip_cores);
void s32_reset_core(uint8_t part, uint8_t core);
void s32_disable_cofb_clk(uint8_t part, uint32_t keep_blocks);
bool s32_is_interconnect_disabled(uint32_t part);
void s32_enable_interconnect(uint32_t part);
void s32_disable_interconnect(uint32_t part);
const struct a53_haddr_mapping *s32_get_a53_haddr_mappings(size_t *size);
void mc_me_enable_partition_block(uint32_t part, uint32_t block);
int mc_me_enable_partition(uint32_t part);
int mc_me_disable_partition(uint32_t part);
void mc_me_part_pupd_update_and_wait(uint32_t part, uint32_t mask);
uint8_t mc_me_core2prtn_core_id(uint8_t part, uint8_t id);
uint32_t mc_me_get_cluster_ptrn(uint32_t core);

void s32_destructive_reset(void);

#endif /* S32CC___S32_MC_ME_H__ */

