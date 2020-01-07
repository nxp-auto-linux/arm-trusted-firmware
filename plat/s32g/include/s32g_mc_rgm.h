/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __S32G_MC_RGM_H__
#define __S32G_MC_RGM_H__

#define S32G_MC_RGM_BASE_ADDR	0x40078000ul
#define S32G_MC_RGM_SIZE	0x1000ul
#define S32G_MC_RGM_PRST_BASE_ADDR	(S32G_MC_RGM_BASE_ADDR + 0x40)
#define S32G_MC_RGM_PSTAT_BASE_ADDR	(S32G_MC_RGM_BASE_ADDR + 0x140)
/* Peripheral reset */
#define S32G_MC_RGM_PRST(p)	(S32G_MC_RGM_PRST_BASE_ADDR + 0x8 * p)
#define S32G_MC_RGM_PSTAT(p)	(S32G_MC_RGM_PSTAT_BASE_ADDR + 0x8 * p)

/* Software-resettable domain/partition 1: CA53 cores */
#define S32G_MC_RGM_RST_DOMAIN_CA53	1
/* Bit corresponding to CA53_n in the cores' RGM reset partition (n=0..3) */
#define S32G_MC_RGM_RST_CA53_BIT(n)	BIT(n + 1)
/* The entire domain defined by S32G_MC_RGM_RST_DOMAIN_CA53 can be reset */
#define S32G_MC_RGM_RST_CA53_PART_BIT	BIT(0)

#endif /* __S32G_MC_RGM_H__ */
