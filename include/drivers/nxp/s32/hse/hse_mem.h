/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2024 NXP
 */

#ifndef HSE_MEM_H
#define HSE_MEM_H

#include <stddef.h>
#include <stdint.h>

int hse_mem_setup(uintptr_t base_addr, size_t mem_size);
void *hse_mem_alloc(size_t size);
void hse_mem_free(void *addr);
void *hse_memcpy(void *dest, const void *src, size_t size);
uintptr_t hse_virt_to_phys(const void *addr);

#endif /* HSE_MEM_H */
