/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <platform.h>
#include <platform_def.h>

unsigned int plat_get_syscnt_freq2(void)
{
	return COUNTER_FREQUENCY;
}

