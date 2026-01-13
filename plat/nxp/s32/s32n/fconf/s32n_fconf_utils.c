/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <errno.h>

#include <s32n_fconf_utils.h>

int fconf_get_node_info(const void *dtb, int offset, const char *compatible,
		uintptr_t *base, size_t *size)
{
	int node, ret;

	if (!dtb || !base || !size)
		return -EINVAL;

	if (offset < 0 && !compatible)
		return -EINVAL;

	/* search node by compatible starting from offset */
	if (compatible) {
		node = fdt_node_offset_by_compatible(dtb, offset, compatible);
		if (node < 0) {
			ERROR("FCONF: Missing node with %s compatible.\n", compatible);
			return node;
		}
	} else {
		/* use the provided offset directly */
		node = offset;
	}

	ret = fdt_get_reg_props_by_index(dtb, node, 0, base, size);
	if (ret < 0) {
		ERROR("FCONF: Fail to read node reg property\n");
		return ret;
	}

	return node;
}
