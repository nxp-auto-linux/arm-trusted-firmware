/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * This is based on plat/nxp/common/setup/ls_interrupt_mgmt.c
 */
#include <bl31/interrupt_mgmt.h>
#include <common/debug.h>
#include <plat/common/platform.h>

#include <platform_def.h>
#include <assert.h>

static interrupt_type_handler_t type_el3_interrupt_table[S32CC_MAX_IRQ];

int request_intr_type_el3(uint32_t id, interrupt_type_handler_t handler)
{
	/* Validate 'handler' and 'id' parameters */
	if (!handler || id >= ARRAY_SIZE(type_el3_interrupt_table)) {
		return -EINVAL;
	}

	/* Check if a handler has already been registered */
	if (type_el3_interrupt_table[id] != NULL) {
		return -EALREADY;
	}

	type_el3_interrupt_table[id] = handler;

	return 0;
}

static uint64_t s32cc_el3_irq_handler(uint32_t id, uint32_t flags,
				      void *handle, void *cookie)
{
	uint32_t intr_id;
	interrupt_type_handler_t handler;

	intr_id = plat_ic_acknowledge_interrupt();
	intr_id = plat_ic_get_interrupt_id(intr_id);

	if (intr_id >= ARRAY_SIZE(type_el3_interrupt_table)) {
		goto exit;
	}

	handler = type_el3_interrupt_table[intr_id];
	if (handler != NULL) {
		handler(intr_id, flags, handle, cookie);
	}

exit:
	/*
	 * Mark this interrupt as complete to avoid a interrupt storm.
	 */
	plat_ic_end_of_interrupt(intr_id);

	return 0U;
}

void s32cc_el3_interrupt_config(void)
{
	uint64_t flags = 0U;
	uint64_t rc;

	set_interrupt_rm_flag(flags, NON_SECURE | SECURE);
	rc = register_interrupt_type_handler(INTR_TYPE_EL3,
					     s32cc_el3_irq_handler, flags);
	if (rc != 0U) {
		panic();
	}
}
