/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>

#include <drivers/nxp/s32n/s32n_cohort_manager.h>
#include <drivers/nxp/s32n/s32n_xrdc_aida.h>
#include <drivers/nxp/s32n/s32n_xrdc_midac.h>

int s32n_cohort_manager_init(uint32_t cohort_id)
{
	int ret;

	if (!S32N_COHORT_MANAGER_SUPPORT)
		return 0;

	ret = s32n_xrdc_aida_init(cohort_id);
	if (ret) {
		ERROR("CM: XRDC AIDA init failed (ret=%d)\n", ret);
		return ret;
	}

	ret = s32n_xrdc_midac_init(cohort_id);
	if (ret) {
		ERROR("CM: XRDC MIDAC init failed (ret=%d)\n", ret);
		return ret;
	}

	return 0;
}
