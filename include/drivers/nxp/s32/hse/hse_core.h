/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2022-2024 NXP
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

int hse_srv_req_sync(enum hse_ch_type channel, const hseSrvDescriptor_t *srv_desc);
int hse_driver_init(void);
bool is_secboot_active(void);

#endif /* HSE_CORE_H */
