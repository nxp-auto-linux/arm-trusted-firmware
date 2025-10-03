/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * NXP HSE2 Driver - MU interface
 *
 * Copyright 2025 NXP
 */

#ifndef HSE_MU_H
#define HSE_MU_H

#define HSE_NUM_CHANNELS    8u /* number of available service channels */

int hse_mu_init(uintptr_t regs_base);

uint16_t hse_mu_check_status(void);

bool hse_mu_channel_available(uint8_t channel);
bool hse_mu_msg_pending(uint8_t channel);

int hse_mu_msg_send(uint8_t channel, uint32_t msg);
int hse_mu_msg_recv(uint8_t channel, uint32_t *msg);

#endif /* HSE_MU_H */
