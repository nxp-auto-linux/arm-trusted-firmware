/*
 * Copyright 2022-2023 NXP
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

typedef struct s32_irq {
	uint32_t id;
	interrupt_type_handler_t handler;
} s32_irq_t;

static s32_irq_t s32_irq_map[S32CC_MAX_IRQ_NUM];
static unsigned int irq_count;

static interrupt_type_handler_t get_irq_handler(uint32_t id)
{
	unsigned int i;

	for (i = 0; i < irq_count; i++) {
		if (id == s32_irq_map[i].id)
			return s32_irq_map[i].handler;
	}

	return NULL;
}

static int set_irq_handler(uint32_t id, interrupt_type_handler_t handler)
{
	if (irq_count >= S32CC_MAX_IRQ_NUM || !handler) {
		return -EINVAL;
	}

	s32_irq_map[irq_count].id = id;
	s32_irq_map[irq_count++].handler = handler;

	return 0;
}

int request_intr_type_el3(uint32_t id, interrupt_type_handler_t handler)
{
	if (get_irq_handler(id) != NULL) {
		return -EALREADY;
	}

	return set_irq_handler(id, handler);
}

static uint64_t s32cc_el3_irq_handler(uint32_t id, uint32_t flags,
				      void *handle, void *cookie)
{
	uint32_t intr_id;
	interrupt_type_handler_t handler;

	intr_id = plat_ic_acknowledge_interrupt();
	intr_id = plat_ic_get_interrupt_id(intr_id);

	handler = get_irq_handler(intr_id);
	if (handler != NULL) {
		handler(intr_id, flags, handle, cookie);
	}

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
