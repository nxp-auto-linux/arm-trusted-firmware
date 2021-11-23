/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/desc_image_load.h>
#include <lib/libc/errno.h>
#include "s32_bl_common.h"
#include "s32_bl2_el3.h"
#include "s32_linflexuart.h"
#include "s32_storage.h"

static bl_mem_params_node_t s32r_bl2_mem_params_descs[6];
REGISTER_BL_IMAGE_DESCS(s32r_bl2_mem_params_descs)

void bl2_el3_early_platform_setup(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	size_t index = 0;
	bl_mem_params_node_t *params = s32r_bl2_mem_params_descs;

	s32_early_plat_init(false);
	console_s32_register();
	s32_io_setup();

	add_fip_img_to_mem_params_descs(params, &index);
	add_bl31_img_to_mem_params_descs(params, &index);
	add_bl32_img_to_mem_params_descs(params, &index);
	add_bl32_extra1_img_to_mem_params_descs(params, &index);
	add_bl33_img_to_mem_params_descs(params, &index);
	add_invalid_img_to_mem_params_descs(params, &index);

	bl_mem_params_desc_num = index;
}

void bl2_el3_plat_arch_setup(void)
{
	__asm__ volatile("b .");
}

