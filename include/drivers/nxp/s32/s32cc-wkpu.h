// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/*
 * Copyright 2020, 2022, 2025 NXP
 */
#ifndef S32CC_WKPU_H
#define S32CC_WKPU_H

int s32cc_wkpu_init(void *fdt, int fdt_offset);
void s32cc_wkpu_reset(void);
void s32cc_wkpu_enable_irqs(void);

#endif
