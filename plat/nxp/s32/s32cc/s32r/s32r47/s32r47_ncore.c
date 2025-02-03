/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "platform_def.h"
#include "s32cc_bl_common.h"
#include "s32cc_ncore.h"

void s32_configure_ncore(void)
{
	if (s32cc_has_ncore()) {
		s32_restore_ncore_caiu(A53_CLUSTER0_CAIU);
		s32_restore_ncore_caiu(A53_CLUSTER1_CAIU);
		ncore_init();
		ncore_caiu_online(A53_CLUSTER0_CAIU);
		ncore_caiu_online(A53_CLUSTER1_CAIU);
	}
}
