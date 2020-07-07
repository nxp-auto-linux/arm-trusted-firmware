/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <drivers/nxp/s32g/ddr/ddrss.h>
#include <plat/common/platform.h>
#include <platform_def.h>
#include <s32g_clocks.h>
#include <ssram_mailbox.h>
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

void bl31ssram_main(void)
{
	extern struct ddrss_conf ddrss_conf;
	s32g_warm_entrypoint_t bl31_warm_entrypoint;
	uintptr_t csr_addr;

	bl31_warm_entrypoint = s32g_ssram_mailbox.bl31_warm_entrypoint;
	csr_addr = (uintptr_t)&s32g_ssram_mailbox.csr_settings[0];

	s32g_plat_ddr_clock_init();
	ddrss_to_normal_mode(&ddrss_conf, csr_addr);

	/* To be debugged */
	__asm__ volatile("bl .");
	bl31_warm_entrypoint();

	/*
	 * This forces the linker to keep s32g_ssram_ivt
	 * and s32g_boot_code symbols.
	 */
	printf("SSRAM IVT address = 0x%p\n", &s32g_ssram_ivt);
	panic();
}
