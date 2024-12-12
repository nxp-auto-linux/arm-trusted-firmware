/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "platform_def.h"
#include "s32cc_bl_common.h"
#include "s32cc_bl2_el3.h"
#include "s32cc_linflexuart.h"
#include "s32cc_storage.h"


void bl2_el3_plat_arch_setup(void)
{
	console_s32_register();

	NOTICE("Reset status: %s\n", get_reset_cause_str());

	if (s32_el3_mmu_ddr_fixup())
		panic();

	s32_io_setup();
}

