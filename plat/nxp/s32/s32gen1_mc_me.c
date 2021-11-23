/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <s32_mc_me.h>

static const struct a53_haddr_mapping haddr_map[] = {
	[0] = { .reg = GPR09_OFF, .field_off = CA53_0_0_RVBARADDR_39_32_OFF, },
	[1] = { .reg = GPR09_OFF, .field_off = CA53_0_1_RVBARADDR_39_32_OFF, },
	[2] = { .reg = GPR09_OFF, .field_off = CA53_1_0_RVBARADDR_39_32_OFF, },
	[3] = { .reg = GPR09_OFF, .field_off = CA53_1_1_RVBARADDR_39_32_OFF, },
};

const struct a53_haddr_mapping *s32_get_a53_haddr_mappings(size_t *size)
{
	*size = ARRAY_SIZE(haddr_map);
	return &haddr_map[0];
}

uint8_t mc_me_core2prtn_core_id(uint8_t part, uint8_t id)
{
	return id;
}

void s32_turn_off_mcores(void)
{
	s32_turn_off_core(S32_MC_ME_CM7_PART, 2);
	s32_turn_off_core(S32_MC_ME_CM7_PART, 1);
	s32_turn_off_core(S32_MC_ME_CM7_PART, 0);
}

uint32_t mc_me_get_cluster_ptrn(uint32_t core)
{
	/**
	 * For S32G2 we have the following mapping:
	 *     MC_ME_PRTN1_CORE0_* -> CA53 cluster0 core0/1
	 *     MC_ME_PRTN1_CORE2_* -> CA53 cluster1 core0/1
	 */
	return (core % 4) & ~1;
}
