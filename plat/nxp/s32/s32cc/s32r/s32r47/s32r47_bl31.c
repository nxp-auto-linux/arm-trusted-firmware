/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <bl31/bl31.h>		/* for bl31_warm_entrypoint() */
#include <clk/clk.h>
#include <lib/mmio.h>
#include <drivers/arm/gicv3.h>
#include <plat/common/platform.h>
#include "platform_def.h"
#include "s32cc_bl_common.h"
#include "s32_bl31_common.h"
#include "s32cc_psci.h"
#include "s32cc_lowlevel.h"

#undef DDRC_BASE
#define DDRC_BASE 0x45800000UL
#define DDRC_ERR_EN 0x1000UL
#define DDRC_PAR_1_DIS 0xC3

extern void s32r47_bl31_spin_loop(void);

int dt_init_plat_clk(void *fdt)
{
	return 0;
}

void s32_bl31_early_platform_setup(void)
{
	/*
	 * Disable DDRC ERR_EN[PAR_1_EN] register field.
	 */
	mmio_write_32((uintptr_t)(DDRC_BASE + DDRC_ERR_EN), DDRC_PAR_1_DIS);

	/* Release the other cores from the BL2 spintable
	 * and put them into another spintable but inside
	 * BL31. This must happen before the MMU changes
	 * since the BL2 spintable is expected to be
	 * mapped as NON_CACHEABLE.
	 */
	uintptr_t *jump_address =
		(uintptr_t *)(S32R47_BL2_SPIN_TABLE_ADDR + PLATFORM_CORE_COUNT +
			      S32R47_SPIN_TABLE_RSRV);
	uint8_t *spintable_address = (uint8_t *)S32R47_BL2_SPIN_TABLE_ADDR;
	int i;

	for (i = 0; i < PLATFORM_CORE_COUNT; i++) {
		jump_address[i] = (uintptr_t)s32r47_bl31_spin_loop;
	}

	dsbsy();

	for (i = 0; i < PLATFORM_CORE_COUNT; i++) {
		spintable_address[i] = S32R47_SPIN_TABLE_GO;
	}

	dsbsy();
	sev();
}
