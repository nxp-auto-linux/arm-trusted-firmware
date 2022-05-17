/*
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <plat/common/platform.h>
#include <drivers/nxp/s32/ddr/ddr_lp.h>
#include <platform_def.h>
#include <s32g_clocks.h>
#include <plat/nxp/s32g/bl31_ssram/ssram_mailbox.h>
#include <s32g_ivt.h>

#include "bl31_ssram.h"

struct ssram_ivt_sec s32g_ssram_ivt __section(".ssram_ivt1") = {
	.ivt = {
		.tag = IVT_TAG,
		.length = IVT_LEN,
		.version = APPLICATION_BOOT_CODE_VERSION,
		.application_boot_code_pointer = BL31SSRAM_IVT +
			sizeof(struct s32gen1_ivt),
		.boot_configuration_word = BCW_BOOT_TARGET_A53_0,
	},
	.app_code = {
		.tag = APPLICATION_BOOT_CODE_TAG,
		.version = APPLICATION_BOOT_CODE_VERSION,
		.ram_start_pointer = BL31SSRAM_BASE,
		.ram_entry_pointer = BL31SSRAM_BASE,
		.code_length = S32G_SSRAM_LIMIT - BL31SSRAM_BASE,
	},
};

struct s32g_ssram_mailbox s32g_ssram_mailbox __section(".mailbox");

#if S32CC_EMU == 0
static void ddr_resume(void)
{
	uintptr_t csr_addr;

	csr_addr = (uintptr_t)&s32g_ssram_mailbox.csr_settings[0];

	s32g_plat_ddr_clock_init();

	if (ddrss_to_normal_mode(csr_addr))
		panic();
}
#else
static void ddr_resume(void)
{
}
#endif

void bl31ssram_main(void)
{
	s32g_warm_entrypoint_t s32g_resume_entrypoint;

	s32g_resume_entrypoint = s32g_ssram_mailbox.bl31_warm_entrypoint;

	ddr_resume();
	s32g_resume_entrypoint();

	/*
	 * This forces the linker to keep s32g_ssram_ivt
	 * and s32g_boot_code symbols.
	 */
	printf("SSRAM IVT address = 0x%p\n", &s32g_ssram_ivt);
	panic();
}
