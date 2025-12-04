/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * NXP HSE2 Driver - interface
 *
 * Copyright 2025 NXP
 */

#ifndef HSE_DRV_H
#define HSE_DRV_H

#include "hse_interface.h"

/**
 * enum hse_ch_type - channel type
 * @HSE_CHANNEL_ADMIN: restricted to administrative services
 * @HSE_CHANNEL_CRYPTO: first channel available for crypto services
 * @HSE_CHANNEL_ANY: use any channel available for crypto services
 * @HSE_CHANNEL_INV: invalid channel
 *
 * Except HSE_CHANNEL_ADM (MU instance channel zero), any channel index
 * below HSE_NUM_CHANNELS can be used to request crypto services.
 */
enum hse_ch_type {
	HSE_CHANNEL_ADMIN = 0u,
	HSE_CHANNEL_CRYPTO = 1u,
	HSE_CHANNEL_ANY = 0xAC,
	HSE_CHANNEL_INV = 0xFF,
};

int hse_driver_init(void);

int hse_srv_req_sync(uint8_t channel, const hseSrvDescriptor_t *srv_desc);

void *hse_mem_alloc(size_t size);
void hse_mem_free(void *addr);

void *hse_memcpy(void *dst, const void *src, size_t size);
void *hse_memset(void *dst, uint8_t fill, size_t size);

bool hse_check_accessible_addr(uintptr_t addr);

uintptr_t hse_virt_to_phys(void *addr);

void hse_flush_range(uintptr_t addr, size_t size);
void hse_inval_range(uintptr_t addr, size_t size);

#endif /* HSE_DRV_H */
