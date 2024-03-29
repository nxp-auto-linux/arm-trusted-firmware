/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/bl_common.h>
#include "platform_def.h"

/* Dummy value, to be provided by FSS */
#define COUNTER_FREQUENCY       8000000 /* 8MHz */

static entry_point_info_t bl33_image_ep_info;

void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
				u_register_t arg2, u_register_t arg3)
{

}

void bl31_plat_arch_setup(void)
{

}

void bl31_platform_setup(void)
{

}

void bl31_plat_runtime_setup(void)
{

}

struct entry_point_info *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	return &bl33_image_ep_info;
}

unsigned int plat_get_syscnt_freq2(void)
{
	return COUNTER_FREQUENCY;
}
