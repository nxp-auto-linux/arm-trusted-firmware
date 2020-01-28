/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/mmio.h>
#include "s32g_ncore.h"
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

void ncore_caiu_online(uint32_t caiu)
{
	int numdirus, diru;

	numdirus = CSUIDR_NUMDIRUS(mmio_read_32(CSUIDR));
	for (diru = 0; diru < numdirus; diru++)
		mmio_write_32(DIRUCASER(diru), mmio_read_32(DIRUCASER(diru)) |
						DIRUCASER_CASNPEN(caiu));

	if (caiu == A53_CLUSTER1_CAIU)
		if (mmio_read_32(GPR_BASE_ADDR + GPR06_OFF) & CA53_LOCKSTEP_EN)
			return;

	if ((mmio_read_32(CAIUIDR(caiu)) &
				CAIUIDR_TYPE) == CAIUIDR_TYPE_ACE_DVM)
		mmio_write_32(CSADSER, mmio_read_32(CSADSER) |
							CSADSER_DVMSNPEN(caiu));
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
