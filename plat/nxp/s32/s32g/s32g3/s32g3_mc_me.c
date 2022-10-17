/*
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <s32g_mc_me.h>

static const struct a53_haddr_mapping haddr_map[] = {
	[0] = { .reg = GPR09_OFF, .field_off = CA53_0_0_RVBARADDR_39_32_OFF, },
	[1] = { .reg = GPR09_OFF, .field_off = CA53_0_1_RVBARADDR_39_32_OFF, },
	[2] = { .reg = GPR36_OFF, .field_off = CA53_0_2_RVBARADDR_39_32_OFF, },
	[3] = { .reg = GPR36_OFF, .field_off = CA53_0_3_RVBARADDR_39_32_OFF, },
	[4] = { .reg = GPR09_OFF, .field_off = CA53_1_0_RVBARADDR_39_32_OFF, },
	[5] = { .reg = GPR09_OFF, .field_off = CA53_1_1_RVBARADDR_39_32_OFF, },
	[6] = { .reg = GPR36_OFF, .field_off = CA53_1_2_RVBARADDR_39_32_OFF, },
	[7] = { .reg = GPR36_OFF, .field_off = CA53_1_3_RVBARADDR_39_32_OFF, },
};

const struct a53_haddr_mapping *s32_get_a53_haddr_mappings(size_t *size)
{
	*size = ARRAY_SIZE(haddr_map);
	return &haddr_map[0];
}

uint8_t mc_me_core2prtn_core_id(uint8_t part, uint8_t id)
{
	/**
	 * A map where the key is core id obrained from MPIDR and the
	 * value represents the ID of the core in MC_ME.PRTN1_CORE*
	 */
	static const uint8_t mc_me_a53_core_id[] = {
		/* Cluster 0, core 0 */
		[0] = 0,
		/* Cluster 0, core 1 */
		[1] = 1,
		/* Cluster 0, core 2 */
		[2] = 4,
		/* Cluster 0, core 3 */
		[3] = 5,
		/* Cluster 1, core 0 */
		[4] = 2,
		/* Cluster 1, core 1 */
		[5] = 3,
		/* Cluster 1, core 2 */
		[6] = 6,
		/* Cluster 1, core 3 */
		[7] = 7,
	};

	static const uint8_t mc_me_m7_core_id[] = {
		/* Core 0 */
		[0] = 0,
		[1] = 1,
		[2] = 2,
		/* Core 3 */
		[3] = 4,
	};

	if (part == S32_MC_ME_CA53_PART)
		return mc_me_a53_core_id[id];

	return mc_me_m7_core_id[id];
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
	 * For S32G3 we have the following mapping:
	 *     MC_ME_PRTN1_CORE0_* -> CA53 cluster0 core0/1/2/3
	 *     MC_ME_PRTN1_CORE2_* -> CA53 cluster1 core0/1/2/3
	 */
	return mc_me_core2prtn_core_id(S32_MC_ME_CA53_PART, core) & 2;
}

