// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021,2023-2024 NXP
 */

#ifndef MC_RGM_REGS_H
#define MC_RGM_REGS_H

#include <s32cc_bl_common.h>

#define PRST_PERIPH_n_RST(n)		BIT(n)

#define RGM_PSTAT(rgm, per)		(UPTR(rgm) + 0x140 + \
					 ((per) * 0x8))
#define PSTAT_PERIPH_n_STAT(n)		BIT(n)

#endif /* MC_RGM_REGS_H */

