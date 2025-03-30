/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <libc/errno.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <plat/common/platform.h>
#include <stdint.h>

#include <s32_derating_erratum.h>
#include <s32cc_interrupt_mgmt.h>
#include <tsp_private.h>

#include "ddr_utils.h"

#define DDRC_SIZE		(0x1000)
#define DDRC_PERF_SIZE	(0x20000)

static uint8_t ddr_erratum_polling_flag;

static int s32_mmap_ddr_controller(void)
{
	int result;

	result = mmap_add_dynamic_region(DDRC_BASE_ADDR,
					 DDRC_BASE_ADDR,
					 DDRC_SIZE,
					 MT_DEVICE | MT_RW);

	if (result < 0) {
		ERROR("Error while mapping DDRC: error %d\n", result);
		return -ENOMEM;
	}

	result = mmap_add_dynamic_region(PERF_BASE_ADDR,
					 PERF_BASE_ADDR,
					 DDRC_PERF_SIZE,
					 MT_DEVICE | MT_RW);

	if (result < 0) {
		ERROR("Error while mapping DDR PERF: error %d\n", result);
		return -ENOMEM;
	}

	return 0;
}

void s32_set_ddr_erratum_polling_flag(uint8_t polling_value)
{
	ddr_erratum_polling_flag = polling_value;
}

void s32_ddr_erratum_timer_start(void)
{
	tsp_generic_timer_start();
}

static uint64_t s32_ddr_erratum_irq_handler(uint32_t id, uint32_t flags,
					    void *handle, void *cookie)
{
	int result;

	tsp_generic_timer_stop();

	result = poll_derating_temp_errata();
	if (result == ERRATA_CHANGES_REVERTED) {
		s32_set_ddr_erratum_polling_flag(0);
	} else
		tsp_generic_timer_handler();

	s32_ddr_erratum_timer_start();

	return result;
}

void s32_enable_ddr_erratum_irq(void)
{
	if (ddr_erratum_polling_flag == POLLING_NEEDED_KEY) {
		plat_ic_set_interrupt_type(TSP_IRQ_SEC_PHY_TIMER, INTR_TYPE_EL3);
		plat_ic_enable_interrupt(TSP_IRQ_SEC_PHY_TIMER);
	}
}

void s32_ddr_erratum_interrupt_config(void)
{
	uint64_t flags = 0U;
	int rc;

	if (ddr_erratum_polling_flag != POLLING_NEEDED_KEY)
		return;

	rc = s32_mmap_ddr_controller();
	if (rc != 0)
		panic();

	set_interrupt_rm_flag(flags, NON_SECURE | SECURE);
	rc = register_interrupt_type_handler(INTR_TYPE_EL3,
					     s32_ddr_erratum_irq_handler, flags);
	if (rc != 0)
		panic();

	s32_enable_ddr_erratum_irq();

	s32_ddr_erratum_timer_start();
}

void s32_ddr_erratum_enable_irq(int pos)
{
	if (ddr_erratum_polling_flag == POLLING_NEEDED_KEY) {
		// This will be run only on CPU0.
		if (pos == 0) {
			s32_enable_ddr_erratum_irq();
			s32_ddr_erratum_timer_start();
		}
	}
}

void s32_ddr_erratum_trigger_handler(int intid)
{
	if (ddr_erratum_polling_flag == POLLING_NEEDED_KEY) {
		if (intid == TSP_IRQ_SEC_PHY_TIMER)
			s32_ddr_erratum_irq_handler(0, 0, NULL, NULL);
	}
}
