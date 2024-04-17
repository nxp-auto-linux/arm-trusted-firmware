/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32CC_DT_H
#define S32CC_DT_H

#include <libfdt_env.h>
#include <stdint.h>

#define DT_DISABLED	0
#define DT_ENABLED	1

struct dt_node_info {
	uintptr_t base;
	int32_t clock;
	int32_t reset;
	uint32_t status;
	size_t reg_size;
};

int dt_open_and_check(void);
int fdt_get_address(void **fdt_addr);
uint8_t fdt_get_status(int node);
void dt_fill_device_info(struct dt_node_info *info, int node);
int dt_enable_clocks(void *fdt, int node);
int fdt_read_irq_cells(const fdt32_t *prop, int nr_cells);
int fdt_get_irq_props_by_index(const void *dtb, int node, unsigned int irq_cell_size,
			       unsigned int index, int *irq_num);
int fdt_node_offset_by_prop_found(const void *fdt, int startoffset,
				  const char *propname);

#endif
