/*
 * Copyright 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ddr_init.h"
#include <lib/libc/errno.h>
#include <lib/mmio.h>
#include "platform_def.h"
#include "s32cc_bl_common.h"
#include "s32cc_bl2_el3.h"
#if (ERRATA_S32_050543 == 1)
#include "s32cc_ddr_errata_funcs.h"
#endif
#include "s32cc_linflexuart.h"
#include "s32cc_sramc.h"
#include "s32cc_storage.h"


void bl2_el3_plat_arch_setup(void)
{
	console_s32_register();

	NOTICE("Reset status: %s\n", get_reset_cause_str());

	s32_sram_clear(S32_BL33_IMAGE_BASE, get_bl2_dtb_base());

	clear_swt_faults();

	if (ddr_init()) {
		ERROR("Failed to configure the DDR subsystem\n");
		panic();
	}

	dsbsy();

	if (s32_el3_mmu_ddr_fixup())
		panic();

#if (ERRATA_S32_050543 == 1)
	ddr_errata_update_flag(polling_needed);
#endif

	s32_io_setup();

	hse_secboot_setup();
}

