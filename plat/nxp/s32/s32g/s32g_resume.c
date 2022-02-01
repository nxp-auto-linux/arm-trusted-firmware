/*
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "s32g_bl_common.h"
#include "s32_linflexuart.h"
#include "s32g_lowlevel.h"
#include "s32g_resume.h"
#include "s32g_vr5510.h"
#include "s32gen1-wkpu.h"
#include <bl31/bl31.h>		/* for bl31_warm_entrypoint() */
#include <lib/el3_runtime/context_mgmt.h>
#include <lib/el3_runtime/cpu_data.h>
#include <lib/mmio.h>

void s32_ncore_isol_cluster0(void);

static void reset_rtc(void)
{
	uint32_t rtc = S32G_RTC_BASE;
	uint32_t rtcs;

	mmio_write_32(rtc + RTC_APIVAL_OFFSET, 0x0);
	mmio_write_32(rtc + RTC_RTCVAL_OFFSET, 0x0);

	mmio_write_32(rtc + RTC_RTCC_OFFSET, 0x0);

	rtcs = mmio_read_32(rtc + RTC_RTCS_OFFSET);
	mmio_write_32(rtc + RTC_RTCS_OFFSET, rtcs);
}

void s32g_resume_entrypoint(void)
{
	int ret;

	/* Prepare resume operation */
	reset_registers_for_lockstep();
	s32_ncore_isol_cluster0();
	s32_early_plat_init(true);

	ret = pmic_setup();
	if (ret)
		ERROR("Failed to disable VR5510 watchdog\n");

	reset_rtc();
	s32gen1_wkpu_reset();

#if (S32_USE_LINFLEX_IN_BL31 == 1)
	console_s32_register();
#endif
	plat_gic_restore();
	bl31_warm_entrypoint();
}
