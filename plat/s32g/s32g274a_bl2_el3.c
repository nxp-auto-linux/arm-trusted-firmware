/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform.h>
#include <common/bl_common.h>
#include <s32g_ncore.h>
#include <drivers/console.h>
#include "s32g_pinctrl.h"
#include "s32g_clocks.h"
#include "s32g_linflexuart.h"


void bl2_platform_setup(void)
{
	return;
}

struct bl_params *plat_get_next_bl_params(void)
{
	/* TODO */
	return NULL;
}

void plat_flush_next_bl_params(void)
{
	/* TODO */
}

struct bl_load_info *plat_get_bl_image_load_info(void)
{
	/* TODO */
	return NULL;
}

void bl2_el3_early_platform_setup(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	s32g_plat_config_pinctrl();
	s32g_plat_clock_init();

	ncore_init();
	ncore_caiu_online(A53_CLUSTER0_CAIU);
}

void bl2_el3_plat_arch_setup(void)
{
	static struct console_s32g console;

	console_s32g_register(S32G_UART_BASE, S32G_UART_CLOCK_HZ,
			      S32G_UART_BAUDRATE, &console);

}

int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	/* TODO */
	return 0;
}
