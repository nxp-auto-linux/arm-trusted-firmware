/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat/common/platform.h>

int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	return 0;
}

int io_open(uintptr_t dev_handle, const uintptr_t spec, uintptr_t *handle)
{
	return 0;
}

int io_size(uintptr_t handle, size_t *length)
{
	return 0;
}

int io_read(uintptr_t handle, uintptr_t buffer, size_t length,
	    size_t *length_read)
{
	return 0;
}

int io_close(uintptr_t handle)
{
	return 0;
}

int io_dev_close(uintptr_t dev_handle)
{
	return 0;
}

void plat_secondary_cold_boot_setup(void)
{
}

void platform_mem_init(void)
{
}

void bl1_early_platform_setup(void)
{
}

void bl1_plat_arch_setup(void)
{
}

void bl1_platform_setup(void)
{
}

struct meminfo *bl1_plat_sec_mem_layout(void)
{
	return NULL;
}
