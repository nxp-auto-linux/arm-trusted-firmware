/*
 * Copyright 2019-2021, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32CC_NCORE_H
#define S32CC_NCORE_H

#ifndef __ASSEMBLER__
#include <arch_helpers.h>
#endif
#include "platform_def.h"

#define NCORE_BASE_ADDR		(0x50400000)
#define S32_NCORE_SIZE		SIZE_1M

#define A53_CLUSTER0_CAIU	(0)
#define A53_CLUSTER1_CAIU	(1)

#define DIRU(n)			(NCORE_BASE_ADDR + 0x80000 + (n) * 0x1000)
#define DIRUSFER(n)		(DIRU(n) + 0x10)
#define DIRUSFER_SFEN(sf)	BIT(sf)
#define DIRUCASER(n) (DIRU(n) + 0x40)
#define DIRUCASER_NCBU(n)	(DIRU(n) + 0x40 + 0xc)
#define DIRUCASER_CASNPEN(caiu)	BIT(caiu)
#define DIRUSFMAR(n)		(DIRU(n) + 0x84)
#define DIRUSFMAR_MNTOPACTV	BIT(0)
#define DIRUSFMCR(n)		(DIRU(n) + 0x80)
#define DIRUSFMCR_SFID(sf)	((sf) << 16)
#define DIRUSFMCR_SFMNTOP_INITIALIZE_ALL_ENTRIES	(0x0)

#define CAIU(n)                 (NCORE_BASE_ADDR + (n * 0x1000))
#define CAIUIDR(n)		(CAIU(n) + 0xffc)
#define CAIUIDR_TYPE		(0xf << 16)
#define CAIUIDR_TYPE_ACE_DVM	(0x0 << 16)

#define	CSR_OFF			(0xff000)
#define CSR			(NCORE_BASE_ADDR + CSR_OFF)
#define CSADSER_OFF		(0x40)
#define CSADSER			(CSR + CSADSER_OFF)
#define CSADSER_DVMSNPEN(caiu)	BIT(caiu)
#define CSIDR			(CSR + 0xffc)
#define CSIDR_NUMSFS_SHIFT	(18)
#define CSIDR_NUMSFS_MASK	(0x1f << CSIDR_NUMSFS_SHIFT)
#define CSIDR_NUMSFS(csidr)	(((csidr) & CSIDR_NUMSFS_MASK) \
				 >> CSIDR_NUMSFS_SHIFT)
#define CSUIDR			(CSR + 0xff8)
#define CSUIDR_NUMCMIUS_SHIFT	(24)
#define CSUIDR_NUMCMIUS_MASK	(0x3f << CSUIDR_NUMCMIUS_SHIFT)
#define CSUIDR_NUMCMIUS(csuidr)	(((csuidr) & CSUIDR_NUMCMIUS_MASK) \
				 >> CSUIDR_NUMCMIUS_SHIFT)
#define CSUIDR_NUMDIRUS_SHIFT	(16)
#define CSUIDR_NUMDIRUS_MASK	(0x3f << CSUIDR_NUMDIRUS_SHIFT)
#define CSUIDR_NUMDIRUS(csuidr)	(((csuidr) & CSUIDR_NUMDIRUS_MASK) \
				 >> CSUIDR_NUMDIRUS_SHIFT)
#define CSUIDR_NUMNCBUS_SHIFT	(8)
#define CSUIDR_NUMNCBUS_MASK	(0x3f << CSUIDR_NUMNCBUS_SHIFT)
#define CSUIDR_NUMNCBUS(csuidr)	(((csuidr) & CSUIDR_NUMNCBUS_MASK) \
				 >> CSUIDR_NUMNCBUS_SHIFT)

#define CA53_LOCKSTEP_EN	BIT(0)

#ifndef __ASSEMBLER__
void ncore_caiu_online(uint32_t caiu);
void ncore_caiu_offline(uint32_t caiu);
void ncore_init(void);
bool ncore_is_caiu_online(uint32_t caiu);
#endif

#endif /* S32CC_NCORE_H */
