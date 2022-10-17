/*
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <s32gen1-wkpu.h>
#include <s32_linflexuart.h>
#include <bl31/bl31.h>		/* for bl31_warm_entrypoint() */

void s32g_resume_entrypoint(void)
{
	s32gen1_wkpu_reset();

#if (S32_USE_LINFLEX_IN_BL31 == 1)
	console_s32_register();
#endif

	bl31_warm_entrypoint();
}
