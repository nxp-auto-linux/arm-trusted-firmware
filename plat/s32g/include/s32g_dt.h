/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G_DT_H
#define S32G_DT_H

#define DT_DISABLED	0
#define DT_ENABLED	1

struct dt_node_info {
	uint32_t base;
	int32_t clock;
	int32_t reset;
	uint32_t status;
};

int dt_open_and_check(void);
int fdt_get_address(void **fdt_addr);
uint8_t fdt_get_status(int node);
void dt_fill_device_info(struct dt_node_info *info, int node);

#endif
