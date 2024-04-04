/*
 * Copyright 2022, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32CC_IRQ_MGMT_H
#define S32CC_IRQ_MGMT_H

#include <bl31/interrupt_mgmt.h>

#define MAX_INTR_EL3		128

/*
 * Register handler to specific GIC entrance
 * for INTR_TYPE_EL3 type of interrupt
 */
int request_intr_type_el3(uint32_t id, interrupt_type_handler_t handler);

void s32cc_el3_interrupt_config(void);

#endif
