/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <bl31/bl31.h>		/* for bl31_warm_entrypoint() */
#include <drivers/arm/gicv3.h>
#include <plat/common/platform.h>
#include "platform_def.h"
#include "s32cc_bl_common.h"
#include "s32cc_ncore.h"
#include "s32cc_psci.h"

int s32_pwr_domain_on(u_register_t mpidr)
{
	/* Empty for the moment */
	int pos = plat_core_pos_by_mpidr(mpidr);

	if (pos < 0)
		return PSCI_E_INTERN_FAIL;

	/* Do some chores on behalf of the secondary core. ICC setup must be
	 * done by the secondaries, because the interface is not memory-mapped.
	 */
	gicv3_rdistif_init(pos);

	/* Kick the secondary core out of wfi */
	NOTICE("S32 TF-A: %s: booting up core %d (%u)\n", __func__, pos,
	       get_core_state(pos, CPU_USE_WFI_FOR_SLEEP));

	if (s32cc_has_ncore() && is_core_in_secondary_cluster(pos) &&
	    !ncore_is_caiu_online(A53_CLUSTER1_CAIU))
		ncore_caiu_online(A53_CLUSTER1_CAIU);

	if (s32cc_has_ncore() && !is_core_in_secondary_cluster(pos) &&
	    !ncore_is_caiu_online(A53_CLUSTER0_CAIU))
		ncore_caiu_online(A53_CLUSTER0_CAIU);

	update_core_state(pos, CPUIF_EN | CPU_ON, CPU_ON);

	dsbsy();
	sev();

	return PSCI_E_SUCCESS;
}
