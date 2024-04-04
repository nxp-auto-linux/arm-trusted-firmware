/*
 * MC Mode Entry definitions for S32G274A and compatible SoCs
 *
 * Copyright 2019-2022, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __S32G_MC_ME_H__
#define __S32G_MC_ME_H__

#include "s32cc_mc_me.h"

#define MC_ME_MODE_CONF_STANDBY	BIT(15)

#define MC_ME_MODE_STAT		(S32_MC_ME_BASE_ADDR + 0xc)
#define MODE_STAT_PREV_MODE	BIT(0)

#define MC_ME_MAIN_COREID	(S32_MC_ME_BASE_ADDR + 0x10)
#define MC_ME_COREID_PIDX(n)	((n) << 8)
#define MC_ME_COREID_CIDX(n)	((n) << 0)

#define S32G_MC_ME_PRIMARY_CORE_MASK	((1ul << S32_PLAT_PRIMARY_CPU) & 0xF)
#define S32G_MC_ME_SECONDARY_CORE_MASK	(~S32G_MC_ME_PRIMARY_CORE_MASK & 0xF)

#define S32G_MC_ME_PFE_PART	S32_MC_ME_PRTN2

/* Standby master core: A53, cluster 0, core 0*/
#define S32G_STBY_MASTER_CORE	0
#define S32G_STBY_MASTER_PART	1

void s32g_set_stby_master_core(uint8_t part, unsigned int core);

#endif /* __S32G_MC_ME_H__ */
