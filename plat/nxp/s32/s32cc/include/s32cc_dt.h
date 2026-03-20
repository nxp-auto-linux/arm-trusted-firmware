/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 * Copyright 2020-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32CC_DT_H
#define S32CC_DT_H

#include <libfdt_env.h>
#include <stdint.h>

struct dt_node_info {
	uintptr_t base;
	int32_t clock;
	int32_t reset;
	uint32_t status;
	size_t reg_size;
};

int dt_open_and_check(void);
int fdt_get_address(void **fdt_addr);
void dt_fill_device_info(struct dt_node_info *info, int node);
int dt_enable_clocks(void *fdt, int node);
int fdt_node_offset_by_prop_found(const void *fdt, int startoffset,
				  const char *propname);

#endif
