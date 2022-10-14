/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2022 NXP
 */
#ifndef S32GEN1_SCMI_PERF_H
#define S32GEN1_SCMI_PERF_H

#include <clk/s32gen1_clk_funcs.h>
#include <stddef.h>
#include <stdint.h>

#define KHZ                    		(1000U)

#define SCMI_PERF_SET_LIMITS   		BIT(31)
#define SCMI_PERF_SET_LEVEL    		BIT(30)

#define S32GEN1_SCMI_MAX_LEVELS		S32GEN1_MAX_NUM_FREQ

#define rate2khz(rate)				((rate) / KHZ)
#define rate2level(rate)			((rate) - (rate) % KHZ)

int32_t s32gen1_scmi_get_perf_levels(unsigned int agent_id, unsigned int clock_id,
	unsigned int domain_id, size_t lvl_index, uint32_t *levels, size_t *num_levels);

#endif
