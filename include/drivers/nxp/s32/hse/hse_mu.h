/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2022, 2024 NXP
 */

#ifndef HSE_MU_H
#define HSE_MU_H

#include <stdlib.h>
#include <stdint.h>

int hse_mu_init(void);

int hse_mu_msg_recv(uint8_t channel, uint32_t *msg);
int hse_mu_msg_send(uint8_t channel, uint32_t msg);
int hse_mu_msg_pending(uint8_t channel, bool *is_pending);
uint16_t hse_mu_check_status(void);

uintptr_t hse_mu_desc_base_vaddr(void);
uintptr_t hse_mu_desc_base_paddr(void);

#endif /* HSE_MU_H */
