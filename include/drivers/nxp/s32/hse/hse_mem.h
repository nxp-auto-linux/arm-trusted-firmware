/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2025 NXP
 */

#ifndef HSE_MEM_H
#define HSE_MEM_H

#include <stddef.h>
#include <stdint.h>

int hse_mem_setup(uintptr_t base_addr, size_t mem_size);
bool hse_check_accessible_addr(uintptr_t addr);
void *hse_mem_alloc(size_t size);
void hse_mem_free(void *addr);
void *hse_memcpy(void *dest, const void *src, size_t size);
uintptr_t hse_virt_to_phys(const void *addr);
void hse_flush_range(uintptr_t addr, size_t size);
void hse_inval_range(uintptr_t addr, size_t size);

#endif /* HSE_MEM_H */
