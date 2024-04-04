/*
 * Copyright 2019-2021, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/mmio.h>
#include <s32cc_bl_common.h>
#include "s32cc_ncore.h"
#include "platform_def.h"

static void ncore_diru_online(uint32_t diru)
{
	int numsfs, sf;

	numsfs = CSIDR_NUMSFS(mmio_read_32(CSIDR)) + 1;
	for (sf = 0; sf < numsfs; sf++) {
		mmio_write_32(DIRUSFMCR(diru), DIRUSFMCR_SFID(sf) |
				DIRUSFMCR_SFMNTOP_INITIALIZE_ALL_ENTRIES);
		while (mmio_read_32(DIRUSFMAR(diru)) & DIRUSFMAR_MNTOPACTV)
			;
		mmio_write_32(DIRUSFER(diru),
			      mmio_read_32(DIRUSFER(diru)) | DIRUSFER_SFEN(sf));
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

static void set_caiu(uint32_t caiu, bool on)
{
	int numdirus, diru;
	uint32_t dirucase, csadser, caiuidr;

	numdirus = CSUIDR_NUMDIRUS(mmio_read_32(CSUIDR));
	for (diru = 0; diru < numdirus; diru++) {
		dirucase = mmio_read_32(DIRUCASER(diru));

		if (on)
			dirucase |= DIRUCASER_CASNPEN(caiu);
		else
			dirucase &= ~DIRUCASER_CASNPEN(caiu);

		mmio_write_32(DIRUCASER(diru), dirucase);
	}

	if (caiu == A53_CLUSTER1_CAIU && is_lockstep_enabled())
		return;

	caiuidr = mmio_read_32(CAIUIDR(caiu));

	if ((caiuidr & CAIUIDR_TYPE) == CAIUIDR_TYPE_ACE_DVM) {
		csadser = mmio_read_32(CSADSER);

		if (on)
			csadser |= CSADSER_DVMSNPEN(caiu);
		else
			csadser &= ~CSADSER_DVMSNPEN(caiu);

		mmio_write_32(CSADSER, csadser);
	}

}

void ncore_caiu_online(uint32_t caiu)
{
	set_caiu(caiu, true);
}

void ncore_caiu_offline(uint32_t caiu)
{
	set_caiu(caiu, false);
}

bool ncore_is_caiu_online(uint32_t caiu)
{
	uint32_t stat = mmio_read_32(CSADSER);

	return ((stat & CSADSER_DVMSNPEN(caiu)) != 0);
}

void ncore_init(void)
{
	uint32_t numdirus, diru;
	uint32_t numcmius, cmiu;
	uint32_t numncbus, ncbu;
	uint32_t csuidr = mmio_read_32(CSUIDR);

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
