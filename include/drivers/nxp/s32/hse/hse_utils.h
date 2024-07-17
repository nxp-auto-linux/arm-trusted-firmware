/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2024 NXP
 */

#ifndef HSE_UTILS_H
#define HSE_UTILS_H

struct hse_memmap {
	uintptr_t vaddr;
	uintptr_t paddr;
	size_t size;
};

int map_hse_mu_regs(struct hse_memmap *mapping);
int map_hse_mu_desc(struct hse_memmap *mapping);
int map_hse_res_mem(struct hse_memmap *mapping);

#endif /* HSE_UTILS_H */
