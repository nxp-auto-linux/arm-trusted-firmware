/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

#include <drivers/nxp/s32n/s32n_xrdc_common.h>

int s32n_xrdc_mmap_instance(uintptr_t base, size_t size, bool *mapped,
			    const char *name)
{
	int ret;

	if (*mapped) {
		VERBOSE("XRDC-%s: base 0x%lx already mapped\n", name, base);
		return 0;
	}

	ret = mmap_add_dynamic_region(base, base, size,
				      MT_DEVICE | MT_RW | MT_SECURE);
	if (ret) {
		ERROR("XRDC-%s: failed to map base=0x%lx (ret=%d)\n",
		      name, base, ret);
		return ret;
	}

	*mapped = true;
	VERBOSE("XRDC-%s: mapped 0x%lx size=0x%zx\n", name, base, size);

	return 0;
}
