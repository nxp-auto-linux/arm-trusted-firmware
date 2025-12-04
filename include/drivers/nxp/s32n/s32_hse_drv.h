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

#define hse_driver_init(slot)	_hse_driver_init()
int _hse_driver_init(void);

int hse_srv_req_sync(uint8_t channel, const hseSrvDescriptor_t *srv_desc);
int hse_hash_req_set_blen(hseHashSrv_t *hash_req_srv_desc, uint32_t md_size,
			  void **hash_len_buf);
static inline void hse_hash_req_free(void *hash_len_buf)
{}

void *hse_mem_alloc(size_t size);
void *hse_get_accessible_addr(void *addr, size_t len);
void hse_mem_free(void *addr);

void *hse_memcpy(void *dst, const void *src, size_t size);
void *hse_memset(void *dst, uint8_t fill, size_t size);

bool hse_check_accessible_addr(uintptr_t addr);

uintptr_t hse_virt_to_phys(void *addr);

void hse_flush_range(uintptr_t addr, size_t size);
void hse_inval_range(uintptr_t addr, size_t size);

static inline bool is_secboot_active(void)
{
	return true;
}

/* HSE2 vs. HSE1 f/w API wrapper */
#define HSE_SRV_ID_GET_KEY_INFO		HSE_SRV_ID_KEY_GET_INFO

#endif /* HSE_DRV_H */
