/* SPDX-License-Identifier: GPL-2.0 or BSD-3-Clause */
/*
 * Copyright 2020, 2025 NXP
 */

#ifndef DT_S32CC_WKPU_H
#define DT_S32CC_WKPU_H

#include <lib/utils_def.h>

#define S32CC_WKPU_IRQ_RISING		1
#define S32CC_WKPU_IRQ_FALLING		0

#define S32CC_WKPU_PULL_DIS		0
#define S32CC_WKPU_PULL_UP		1
#define S32CC_WKPU_PULL_DOWN		2

/* Interrupt sources: RTC + 23 external */
#define S32CC_WKPU_RTC_IRQ		31
#define S32CC_WKPU_EXT_IRQ(N)		(N)

/* Wakeup Short or Long Boot Select */
#define S32CC_WKPU_SHORT_BOOT		0
#define S32CC_WKPU_LONG_BOOT		1

#endif

