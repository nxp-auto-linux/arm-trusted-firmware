/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <s32g_ncore.h>

#define NCORE_BASE_ADDR		(0x50400000ul)

#define DIRU(n)			(NCORE_BASE_ADDR + 0x80000 + (n * 0x1000))
#define DIRUSFER(n)		(DIRU(n) + 0x10)
#define DIRUSFER_SFEN(sf)	BIT(sf)
#define DIRUCASER_CAIU(n, caiu) (DIRU(n) + 0x40 + (caiu /  32) * 0x4)
#define DIRUCASER_NCBU(n)	(DIRU(n) + 0x40 + 0xc)
#define DIRUCASER_CASNPEN(caiu)	BIT(caiu % 32)
#define DIRUSFMAR(n)		(DIRU(n) + 0x84)
#define DIRUSFMAR_MNTOPACTV	BIT(0)
#define DIRUSFMCR(n)		(DIRU(n) + 0x80)
#define DIRUSFMCR_SFID(sf)	((sf) << 16)
#define DIRUSFMCR_SFMNTOP_INITIALIZE_ALL_ENTRIES	(0x0)

#define CAIU(n)                 (NCORE_BASE_ADDR + (n * 0x1000))
#define CAIUIDR(n)		(CAIU(n) + 0xffc)
#define CAIUIDR_TYPE		(0xf << 16)
#define CAIUIDR_TYPE_ACE_DVM	(0x0 << 16)

#define CSR			(NCORE_BASE_ADDR + 0xff000)
#define CSADSER_CAIU(caiu)	(CSR + 0x40 + (caiu / 32) * 0x4)
#define CSADSER_NCBU		(CSR + 0x40 + 0xc)
#define CSADSER_DVMSNPEN(caiu)	BIT(caiu % 32)
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

#define A53_GPR_BASE_ADDR	(0x4007c400ul)
#define GPR06			(A53_GPR_BASE_ADDR + 0x18)
#define CA53_LOCKSTEP_EN	BIT(0)

#define readl(addr)		(*(uint32_t *)addr)
#define writel(data, addr)	(*(uint32_t *)addr = (uint32_t)data)

static void ncore_diru_online(uint32_t diru)
{
	int numsfs, sf;

	numsfs = CSIDR_NUMSFS(readl(CSIDR)) + 1;
	for (sf = 0; sf < numsfs; sf++) {
		writel(DIRUSFMCR_SFID(sf) |
		       DIRUSFMCR_SFMNTOP_INITIALIZE_ALL_ENTRIES,
		       DIRUSFMCR(diru));
		while (readl(DIRUSFMAR(diru)) & DIRUSFMAR_MNTOPACTV)
			;
		writel(readl(DIRUSFER(diru)) | DIRUSFER_SFEN(sf),
		       DIRUSFER(diru));
	}
}

static void ncore_cmiu_online(uint32_t cmiu)
{
	/* Nothing to be done since the hardware implementation
	 * does not have a coherent memory cache
	 */
}

static void ncore_ncbu_online(uint32_t ncbu)
{
	/* Nothing to be done since the hardware implementation
	 * does not have a proxy cache
	 */
}

void ncore_caiu_online(uint32_t caiu)
{
	int numdirus, diru;

	numdirus = CSUIDR_NUMDIRUS(readl(CSUIDR));
	for (diru = 0; diru < numdirus; diru++)
		writel(readl(DIRUCASER_CAIU(diru, caiu)) |
		       DIRUCASER_CASNPEN(caiu),
		       DIRUCASER_CAIU(diru, caiu));

	if (caiu == A53_CLUSTER1_CAIU)
		if (readl(GPR06) & CA53_LOCKSTEP_EN)
			return;

	if ((readl(CAIUIDR(caiu)) & CAIUIDR_TYPE) == CAIUIDR_TYPE_ACE_DVM)
		writel(readl(CSADSER_CAIU(caiu)) |
		       CSADSER_DVMSNPEN(caiu),
		       CSADSER_CAIU(caiu));
}

void ncore_init(void)
{
	uint32_t numdirus, diru;
	uint32_t numcmius, cmiu;
	uint32_t numncbus, ncbu;
	uint32_t csuidr = readl(CSUIDR);

	numdirus = CSUIDR_NUMDIRUS(csuidr);
	for (diru = 0; diru < numdirus; diru++)
		ncore_diru_online(diru);

	numcmius = CSUIDR_NUMCMIUS(csuidr);
	for (cmiu = 0; cmiu < numcmius; cmiu++)
		ncore_cmiu_online(cmiu);

	numncbus = CSUIDR_NUMNCBUS(csuidr);
	for (ncbu = 0; ncbu < numncbus; ncbu++)
		ncore_ncbu_online(ncbu);
}