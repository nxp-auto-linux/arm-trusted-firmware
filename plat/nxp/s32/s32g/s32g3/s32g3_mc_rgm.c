/*
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <s32g_mc_rgm.h>

uint8_t get_rgm_a53_bit(uint8_t core)
{
	static uint8_t periph_rgm_coresp[] = {
		/** Cluster 0, core 0*/
		[0] = 65,
		[1] = 66,
		[2] = 69,
		[3] = 70,
		/** Cluster 1, core 0*/
		[4] = 67,
		[5] = 68,
		[6] = 71,
		[7] = 72,
	};

	return periph_rgm_coresp[core] % 64;
}

uint8_t get_rgm_m7_bit(uint8_t core)
{
	static uint8_t periph_rgm_mcoresp[] = {
		[0] = 0,
		[1] = 1,
		[2] = 2,
		[3] = 6,
	};

	return periph_rgm_mcoresp[core];
}
