/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2024-2025 NXP
 */

#ifndef HSE_UTILS_H
#define HSE_UTILS_H

#include <s32cc_fconf_hse_getter.h>

int map_hse_mu_regs(struct hse_memmap *map);
int map_hse_mu_desc(struct hse_memmap *map);
int map_hse_res_mem(struct hse_memmap *map, uint32_t slot, bool do_map);

#endif /* HSE_UTILS_H */
