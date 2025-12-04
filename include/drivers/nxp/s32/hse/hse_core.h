/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2022-2025 NXP
 */


#ifndef HSE_CORE_H
#define HSE_CORE_H

#include <stdlib.h>
#include <stdint.h>
#include <hse_interface.h>

/**
 * enum hse_ch_type - channel type
 * @HSE_CHANNEL_ADMIN: restricted to administrative services
 * @HSE_CHANNEL_CRYPTO: channel available for crypto services
 */
enum hse_ch_type {
	HSE_CHANNEL_ADMIN = 0u,
	HSE_CHANNEL_CRYPTO,
	HSE_CHANNEL_NUM
};

enum hse_rmem_slot {
	DEFAULT_RMEM = 0u,
	ALT_RMEM
};

int hse_use_rmem(enum hse_rmem_slot slot);
int hse_srv_req_sync(enum hse_ch_type channel, const hseSrvDescriptor_t *srv_desc);
int hse_hash_req_set_blen(hseHashSrv_t *hash_req_srv_desc, uint32_t md_size,
			  void **hash_len_buf);
void hse_hash_req_free(void *hash_len_buf);
int hse_driver_init(enum hse_rmem_slot slot);
bool hse_addr_in_rmem(uintptr_t addr);
bool is_secboot_active(void);

#endif /* HSE_CORE_H */
