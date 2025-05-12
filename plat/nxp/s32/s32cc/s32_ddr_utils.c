/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>

#include <common/desc_image_load.h>
#include <tbbr_img_def.h>

#include "ddr_init.h"
#include "ddr_utils.h"
#include "s32cc_bl2_el3.h"

void prepare_ddr(void)
{
	bl_mem_params_node_t *bl_mem_params __maybe_unused;
	bl_mem_params_node_t *bl31_params __maybe_unused;

#ifndef CUSTOM_DDR_DRV
	bl_mem_params = get_bl_mem_params_node(DDR_FW_IMAGE_ID);
	assert(bl_mem_params && "bl_mem_params cannot be NULL");
	/* This will also populate CSR section from bl31ssram */
	if (ddr_init(bl_mem_params->image_info.image_base)) {
		ERROR("Failed to configure the DDR subsystem\n");
		panic();
	}
#else
	if (ddr_init()) {
		ERROR("Failed to configure the DDR subsystem\n");
		panic();
	}
#endif

	dsbsy();

	if (s32_el3_mmu_ddr_fixup())
		panic();

#if (ERRATA_S32_050543 == 1)
	bl31_params = get_bl_mem_params_node(BL31_IMAGE_ID);
	bl31_params->ep_info.args.arg1 = polling_needed;
#endif
}
