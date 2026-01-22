/*
 * Copyright 2022, 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32_IRQ_MGMT_H
#define S32_IRQ_MGMT_H

#include <arm/css/scmi/scmi_private.h>
#include <bl31/interrupt_mgmt.h>
#include <common/interrupt_props.h>

#define MAX_INTR_EL3		128
#define S32_MAX_IRQ_NUM		10

/*
 * Register handler to specific GIC entrance
 * for INTR_TYPE_EL3 type of interrupt
 */
int request_intr_type_el3(uint32_t id, interrupt_type_handler_t handler);

interrupt_prop_t *register_and_check_irq(interrupt_prop_t *itr,
					 const interrupt_prop_t *end,
					 interrupt_prop_t *irq_prop);

/*
 * Generic Irq Management function.
 */
void s32_el3_interrupt_config(void);

#endif
