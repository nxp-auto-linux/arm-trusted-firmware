/*
 * Copyright 2021-2022, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <s32cc_mc_me.h>

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

void s32_turn_off_mcores(uint32_t skip_mask)
{
	int i;

	for (i = PLATFORM_M7_CORE_COUNT - 1; i >= 0; i--) {
		if (skip_mask & BIT_32(i))
			continue;

		s32_turn_off_core(S32_MC_ME_CM7_PART, i);
	}
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
