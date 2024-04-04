/*
 * Copyright 2021-2022, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <s32cc_mc_rgm.h>

uint8_t get_rgm_a53_bit(uint8_t core)
{
	/**
	 * Bit corresponding to CA53_n in the cores'
	 * RGM reset partition (n=0..3)
	 */
	return core + 1;
}

uint8_t get_rgm_m7_bit(uint8_t core)
{
	return core;
}
