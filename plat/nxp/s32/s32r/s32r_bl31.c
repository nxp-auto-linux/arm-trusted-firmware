/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <plat/common/platform.h>

void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
		u_register_t arg2, u_register_t arg3)
{
	__asm__ volatile("b .");
}

void bl31_plat_arch_setup(void)
{
	__asm__ volatile("b .");
}

void bl31_platform_setup(void)
{
	__asm__ volatile("b .");
}

entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	__asm__ volatile("b .");
	return NULL;
}
