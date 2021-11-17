// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#ifndef MC_RGM_REGS_H
#define MC_RGM_REGS_H

#include <s32_bl_common.h>

#define RGM_PRST(MC_RGM, per)		(UPTR(MC_RGM) + 0x40 + \
					 ((per) * 0x8))

#define PRST_PERIPH_n_RST(n)		BIT(n)

#define RGM_PSTAT(rgm, per)		(UPTR(rgm) + 0x140 + \
					 ((per) * 0x8))
#define PSTAT_PERIPH_n_STAT(n)		BIT(n)

#endif /* MC_RGM_REGS_H */

