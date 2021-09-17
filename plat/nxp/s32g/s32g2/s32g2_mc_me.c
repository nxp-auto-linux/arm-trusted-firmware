/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <s32g_mc_me.h>

static const struct a53_haddr_mapping haddr_map[] = {
	[0] = { .reg = GPR09_OFF, .field_off = CA53_0_0_RVBARADDR_39_32_OFF, },
	[1] = { .reg = GPR09_OFF, .field_off = CA53_0_1_RVBARADDR_39_32_OFF, },
	[2] = { .reg = GPR09_OFF, .field_off = CA53_1_0_RVBARADDR_39_32_OFF, },
	[3] = { .reg = GPR09_OFF, .field_off = CA53_1_1_RVBARADDR_39_32_OFF, },
};

const struct a53_haddr_mapping *s32g_get_a53_haddr_mappings(size_t *size)
{
	*size = ARRAY_SIZE(haddr_map);
	return &haddr_map[0];
}

uint8_t mc_me_core2prtn_core_id(uint8_t part, uint8_t id)
{
	return id;
}

