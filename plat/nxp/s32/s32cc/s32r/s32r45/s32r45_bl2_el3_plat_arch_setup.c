/*
 * Copyright 2021-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ddr_init.h"
#include <lib/libc/errno.h>
#include <lib/mmio.h>
#include <tbbr_img_def.h>
#include "platform_def.h"
#include "s32cc_bl_common.h"
#include "s32cc_bl2_el3.h"
#include "s32cc_linflexuart.h"
#include "s32cc_sramc.h"
#include "s32cc_storage.h"
#if TRUSTED_BOARD_BOOT
#include <drivers/nxp/s32/hse/hse_core.h>
#endif

void bl2_el3_plat_arch_setup(void)
{
	console_s32_register();

	NOTICE("Reset status: %s\n", get_reset_cause_str());

	s32_sram_clear(S32_BL33_IMAGE_BASE, get_bl2_dtb_base());

	s32_sram_clear(S32_IO_BUFFER_BASE,
		       S32_IO_BUFFER_BASE + S32_IO_BUFFER_SIZE);

	clear_swt_faults();

	hse_secboot_setup();
	hse_sram_rmem_clear();

#ifdef CUSTOM_DDR_DRV
	prepare_ddr();
#endif

	s32_io_setup();
}

int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	bl_mem_params_node_t *bl31_params __maybe_unused;

	if (image_id == BL33_IMAGE_ID) {
		return bl2_copy_bl31_dtb();
	}

	if (image_id == BL32_IMAGE_ID) {
		parse_optee(BL32_IMAGE_ID);
	}

	if (image_id == DDR_FW_IMAGE_ID) {
		/* This will also populate CSR section from bl31ssram */
		configure_ddr(DDR_FW_IMAGE_ID);

		prepare_ddr();
#if TRUSTED_BOARD_BOOT
		hse_use_rmem(ALT_RMEM);
#endif
	}

	return 0;
}
