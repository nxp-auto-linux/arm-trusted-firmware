/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>

#include <drivers/nxp/s32n/s32n_cohort_manager.h>
#include <drivers/nxp/s32n/s32n_xrdc_aida.h>

int s32n_cohort_manager_init(void)
{
	int ret;

	if (!S32N_BL2_AS_COHORT_MANAGER)
		return 0;

	ret = s32n_xrdc_aida_init();
	if (ret) {
		ERROR("CM: XRDC AIDA init failed (ret=%d)\n", ret);
		return ret;
	}

	return 0;
}
