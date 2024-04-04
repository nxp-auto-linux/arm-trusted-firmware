/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <bl31/bl31.h>		/* for bl31_warm_entrypoint() */
#include <s32cc_bl_common.h>
#include <s32cc_linflexuart.h>
#include <s32cc_lowlevel.h>
#include <s32gen1-wkpu.h>
#include <s32cc_scp_scmi.h>

void s32g_resume_entrypoint(void)
{
	uintptr_t core_addr;

	if (!is_scp_used()) {
		s32gen1_wkpu_reset();
	}

#if (S32_USE_LINFLEX_IN_BL31 == 1)
	console_s32_register();
#endif

	if (is_scp_used()) {
		core_addr = (uintptr_t)plat_secondary_cold_boot_setup;
		scp_set_core_reset_addr(core_addr);
	}

	bl31_warm_entrypoint();
}
