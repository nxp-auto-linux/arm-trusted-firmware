// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021, 2023-2024 NXP
 */

#ifndef MC_ME_REGS_H
#define MC_ME_REGS_H

#include <s32cc_bl_common.h>

#define MC_ME_MAX_PARTITIONS		(4u)

/* MC_ME registers. */
#define MC_ME_CTL_KEY(MC_ME)		(UPTR(MC_ME) + 0x0)
#define MC_ME_CTL_KEY_KEY		(0x00005AF0)
#define MC_ME_CTL_KEY_INVERTEDKEY	(0x0000A50F)

/* MC_ME partition definitions */
#define MC_ME_PRTN_N(MC_ME, n)			(UPTR(MC_ME) + 0x100 + \
						 (n) * 0x200)
#define MC_ME_PRTN_N_PCONF(MC_ME, n)		(MC_ME_PRTN_N(MC_ME, n))
#define MC_ME_PRTN_N_PUPD(MC_ME, n)		(MC_ME_PRTN_N(MC_ME, n) + 0x4)
#define MC_ME_PRTN_N_STAT(MC_ME, n)		(MC_ME_PRTN_N(MC_ME, n) + 0x8)
#define MC_ME_PRTN_N_COFB0_STAT(MC_ME, n)	(MC_ME_PRTN_N(MC_ME, n) + 0x10)
#define MC_ME_PRTN_N_COFB0_CLKEN(MC_ME, n)	(MC_ME_PRTN_N(MC_ME, n) + 0x30)

/* MC_ME_PRTN_N_* register fields */
#define MC_ME_PRTN_N_PCE		BIT(0)
#define MC_ME_PRTN_N_PCUD		BIT(0)
#define MC_ME_PRTN_N_PCS		BIT(0)
#define MC_ME_PRTN_N_OSSE		BIT(2)
#define MC_ME_PRTN_N_OSSUD		BIT(2)
#define MC_ME_PRTN_N_REQ(n)		BIT(n)

#define RDC_RD_N_CTRL(RDC, N)	(UPTR(RDC) + (0x4 * (N)))
#define RDC_RD_N_STATUS(RDC, N)	(UPTR(RDC) + 0x80 + (0x4 * (N)))
#define RD_CTRL_UNLOCK_MASK	(0x80000000)
#define RDC_RD_INTERCONNECT_DISABLE BIT(3)
#define RDC_RD_INTERCONNECT_DISABLE_STAT BIT(4)

#endif /* MC_ME_REGS_H */
