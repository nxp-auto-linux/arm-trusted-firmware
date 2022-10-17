/*
 * Copyright 2019-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/mmio.h>
#include "s32g_mc_me.h"

void s32g_set_stby_master_core(uint8_t part, unsigned int core)
{
	/* Set the master core for the standby sequence */
	mmio_write_32(MC_ME_MAIN_COREID,
		      MC_ME_COREID_PIDX(part) |
		      MC_ME_COREID_CIDX(core));

	/* Initiate standby */
	mmio_write_32(MC_ME_MODE_CONF, MC_ME_MODE_CONF_STANDBY);
	mmio_write_32(MC_ME_MODE_UPD, MC_ME_MODE_UPD_UPD);

	/* Write valid key sequence to trigger the update. */
	mc_me_apply_hw_changes();
}

