// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/*
 * Copyright 2020, 2022 NXP
 */
#ifndef S32GEN1_WKPU_H
#define S32GEN1_WKPU_H

int s32gen1_wkpu_init(void *fdt, int fdt_offset);
void s32gen1_wkpu_reset(void);
void s32gen1_wkpu_enable_irqs(void);

#endif
