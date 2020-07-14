/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "s32g_bl_common.h"
#include "s32g_linflexuart.h"
#include "s32g_resume.h"
#include <bl31/bl31.h>		/* for bl31_warm_entrypoint() */
#include <lib/el3_runtime/context_mgmt.h>
#include <lib/el3_runtime/cpu_data.h>

void s32g_ncore_isol_cluster0(void);

void s32g_resume_entrypoint(void)
{
	static struct console_s32g console;

	/* Prepare resume operation */
	s32g_ncore_isol_cluster0();
	s32g_early_plat_init(true);

	console_s32g_register(S32G_UART_BASE, S32G_UART_CLOCK_HZ,
			      S32G_UART_BAUDRATE, &console);
	plat_gic_restore();
	bl31_warm_entrypoint();
}