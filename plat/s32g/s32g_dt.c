/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libfdt.h>
#include <platform_def.h>
#include <s32g_dt.h>

static int fdt_checked;

static void *fdt = (void *)(uintptr_t)DTB_BASE;

int dt_open_and_check(void)
{
	int ret = fdt_check_header(fdt);

	if (ret == 0)
		fdt_checked = 1;

	return ret;
}

int fdt_get_address(void **fdt_addr)
{
	if (fdt_checked == 1)
		*fdt_addr = fdt;

	return fdt_checked;
}

uint8_t fdt_get_status(int node)
{
	uint8_t status = DT_DISABLED;
	int len;
	const char *cchar;

	cchar = fdt_getprop(fdt, node, "status", &len);
	if ((cchar == NULL) ||
			(strncmp(cchar, "okay", (size_t)len) == 0)) {
		status = DT_ENABLED;
	}

	return status;
}

void dt_fill_device_info(struct dt_node_info *info, int node)
{
	const fdt32_t *cuint;

	cuint = fdt_getprop(fdt, node, "reg", NULL);
	if (cuint != NULL)
		info->base = fdt32_to_cpu(*cuint);
	else
		info->base = 0;

	cuint = fdt_getprop(fdt, node, "clocks", NULL);
	if (cuint != NULL) {
		cuint++;
		info->clock = (int)fdt32_to_cpu(*cuint);
	} else
		info->clock = -1;

	cuint = fdt_getprop(fdt, node, "resets", NULL);
	if (cuint != NULL) {
		cuint++;
		info->reset = (int)fdt32_to_cpu(*cuint);
	} else
		info->reset = -1;

	info->status = fdt_get_status(node);
}
