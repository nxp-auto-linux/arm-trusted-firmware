/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 * Copyright 2020-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32_DT_H
#define S32_DT_H

#include <stdint.h>

#define DT_DISABLED	0
#define DT_ENABLED	1

struct dt_node_info {
	uintptr_t base;
	int32_t clock;
	int32_t reset;
	uint32_t status;
};

int dt_open_and_check(void);
int fdt_get_address(void **fdt_addr);
uint8_t fdt_get_status(int node);
void dt_fill_device_info(struct dt_node_info *info, int node);
int dt_enable_clocks(void *fdt, int node);

#endif
