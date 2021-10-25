/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <platform_def.h>
#include <s32g_sramc.h>

#define SRAMC0_MIN_ADDR         (0x0)
#define SRAMC0_MAX_ADDR         (0x7FFF)
#define SRAMC1_MIN_ADDR         (SRAMC0_MAX_ADDR + 1)
#define SRAMC1_MAX_ADDR         (0x10000)

void s32_get_sramc(struct sram_ctrl **ctrls, size_t *size)
{
	static struct sram_ctrl controllers[] = {
		{
			.base_addr = SRAMC0_BASE_ADDR,
			.min_addr = SRAMC0_MIN_ADDR,
			.max_addr = SRAMC0_MAX_ADDR,
		},
		{
			.base_addr = SRAMC1_BASE_ADDR,
			.min_addr = SRAMC1_MIN_ADDR,
			.max_addr = SRAMC1_MAX_ADDR,
		},
	};

	*ctrls = &controllers[0];
	*size = ARRAY_SIZE(controllers);
}

uintptr_t a53_to_sramc_addr(uintptr_t addr)
{
	addr -= S32G_SRAM_BASE;

	/* mem_addr[16:0] = {bus_addr[23:9], bus_addr[5:4]} */
	addr = ((addr >> 9) << 2) | ((addr >> 4) & 0x3);

	return addr;
}

