/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/desc_image_load.h>
#include <lib/libc/errno.h>

static bl_mem_params_node_t s32r_bl2_mem_params_descs[2];
REGISTER_BL_IMAGE_DESCS(s32r_bl2_mem_params_descs)


void bl2_el3_early_platform_setup(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	__asm__ volatile("b .");
}

void bl2_el3_plat_arch_setup(void)
{
	__asm__ volatile("b .");
}

int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	__asm__ volatile("b .");
	return -EINVAL;
}

