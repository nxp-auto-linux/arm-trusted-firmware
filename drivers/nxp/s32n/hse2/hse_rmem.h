/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * NXP HSE2 Driver - memory management
 *
 * Copyright 2025 NXP
 */

#ifndef HSE_RMEM_H
#define HSE_RMEM_H

int hse_rmem_setup(uintptr_t base_addr, size_t mem_size);

void *hse_rmem_alloc(size_t size);
void hse_rmem_free(void *addr);

void *hse_rmem_copy(void *dest, const void *src, size_t size);
void *hse_rmem_set(void *dst, uint8_t fill, size_t size);

#endif /* HSE_RMEM_H */
