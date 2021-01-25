/*
 * Copyright 2019-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DDRSS_H
#define DDRSS_H

#include <ddr/ddr_init.h>
#include <lib/mmio.h>
#include <stddef.h>

void store_csr(uintptr_t store_at);
void ddrss_to_io_lp3_retention_mode(void);
int ddrss_to_normal_mode(uintptr_t csr_array);

#endif
