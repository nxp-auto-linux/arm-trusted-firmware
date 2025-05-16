// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021,2023-2025 NXP
 */

#ifndef MC_RGM_REGS_H
#define MC_RGM_REGS_H

#include <s32cc_bl_common.h>
#include "platform_def.h"

#define PRST_PERIPH_n_RST(n)	BIT_32(n)

#define RGM_PSTAT(rgm, per)	(UPTR(rgm) + 0x140 + ((per) * 0x8))
#define PSTAT_PERIPH_n_STAT(n)	BIT_32(n)

#define RGM_PRST(rgm, per)	(UPTR(rgm) + 0x40UL + (UPTR(per) * 0x8UL))
#define S32_MC_RGM_PRST(p)	(S32_MC_RGM_PRST_BASE_ADDR + 0x8UL * UPTR(p))

#endif /* MC_RGM_REGS_H */

