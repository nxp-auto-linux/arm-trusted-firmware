/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <debug.h>
#include <stdint.h>
#include <common/desc_image_load.h>
#include <plat/common/platform.h>
#include <drivers/arm/pl011.h>
#include <s32n_io_storage.h>

static console_t console;

void bl2_el3_early_platform_setup(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	console_pl011_register(PL011_UART_BASE, PL011_UART_CLOCK_HZ,
		PL011_UART_BAUDRATE, &console);

	NOTICE("BL2: welcome to S32N!\n");

	/* Open handles to a FIP image */
	plat_s32n_io_setup();
}

void bl2_el3_plat_arch_setup(void)
{
}

void bl2_platform_setup(void)
{
}

struct bl_load_info *plat_get_bl_image_load_info(void)
{
	return get_bl_load_info_from_mem_params_desc();
}

struct bl_params *plat_get_next_bl_params(void)
{
	return get_next_bl_params_from_mem_params_desc();
}

void plat_flush_next_bl_params(void)
{
	flush_bl_params_desc();
}
