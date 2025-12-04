/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <s32cc_fconf_hse_getter.h>
#include <s32cc_platform_def.h>
#include <utils_def.h>
#include <s32_hse_drv.h>

int hse_set_rmem_space(const void *fdt)
{
	int ret;

	if (HSE_RMEM_SLOTS_NUM == 1) {
		ret = hse_set_fdt_rmem(fdt, DEFAULT_RMEM);
		if (ret) {
			ERROR("FCONF: Couldn't populate HSE rmem %d\n", ret);
		}
		return ret;
	}

	hse_rmem_memmap[DEFAULT_RMEM].size = round_up(HSE_SRAM_RMEM_BUFFER_SIZE, PAGE_SIZE);
	hse_rmem_memmap[DEFAULT_RMEM].paddr =
				round_down(S32_IO_BUFFER_BASE - hse_rmem_memmap[DEFAULT_RMEM].size,
					PAGE_SIZE);

	ret = hse_set_fdt_rmem(fdt, ALT_RMEM);
	if (ret)
		ERROR("FCONF: Couldn't populate HSE rmem %d\n", ret);

	return ret;
}
