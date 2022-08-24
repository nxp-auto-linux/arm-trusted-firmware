/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2022 NXP
 */
#ifndef S32GEN1_SCMI_PERF_H
#define S32GEN1_SCMI_PERF_H

#include <stddef.h>
#include <stdint.h>

#define KHZ                    	(1000U)

#define SCMI_PERF_SET_LIMITS   	BIT(31)
#define SCMI_PERF_SET_LEVEL    	BIT(30)

#define rate2khz(rate)			((rate) / KHZ)
#define rate2level(rate)		((rate) - (rate) % KHZ)

#endif
