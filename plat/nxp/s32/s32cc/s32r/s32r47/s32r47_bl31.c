/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <bl31/bl31.h>		/* for bl31_warm_entrypoint() */
#include <drivers/arm/gicv3.h>
#include <plat/common/platform.h>
#include "platform_def.h"
#include "s32cc_bl_common.h"
#include "s32_bl31_common.h"
#include "s32cc_psci.h"
#include "s32cc_lowlevel.h"

extern void s32r47_bl31_spin_loop(void);

void s32_bl31_early_platform_setup(void)
{
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
