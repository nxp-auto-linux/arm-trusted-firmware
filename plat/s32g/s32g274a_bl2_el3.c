/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform.h>
#include <common/bl_common.h>
#include <common/desc_image_load.h>
#include <drivers/console.h>
#include "s32g_ncore.h"
#include "s32g_pinctrl.h"
#include "s32g_clocks.h"
#include "s32g_linflexuart.h"
#include "s32g_storage.h"


static bl_mem_params_node_t s32g_bl2_mem_params_descs[] = {
	{
		.image_id = BL31_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      SECURE | EXECUTABLE | EP_FIRST_EXE),
		.ep_info.spsr = SPSR_64(MODE_EL3, MODE_SP_ELX,
					DISABLE_ALL_EXCEPTIONS),
		.ep_info.pc = BL31_BASE,

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, IMAGE_ATTRIB_PLAT_SETUP),
		.image_info.image_max_size = BL31_LIMIT - BL31_BASE,
		.image_info.image_base = BL31_BASE,
		.next_handoff_image_id = INVALID_IMAGE_ID,
	},

	{
		.image_id = INVALID_IMAGE_ID,
		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, IMAGE_ATTRIB_SKIP_LOADING),
	}
};

void bl2_platform_setup(void)
{
	return;
}

struct bl_params *plat_get_next_bl_params(void)
{
	return get_next_bl_params_from_mem_params_desc();
}

void plat_flush_next_bl_params(void)
{
	flush_bl_params_desc();
}

struct bl_load_info *plat_get_bl_image_load_info(void)
{
	return get_bl_load_info_from_mem_params_desc();
}

void bl2_el3_early_platform_setup(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	s32g_plat_config_pinctrl();
	s32g_plat_clock_init();

	ncore_init();
	ncore_caiu_online(A53_CLUSTER0_CAIU);

	s32g_io_setup();
}

void bl2_el3_plat_arch_setup(void)
{
	static struct console_s32g console;

	console_s32g_register(S32G_UART_BASE, S32G_UART_CLOCK_HZ,
			      S32G_UART_BAUDRATE, &console);

}

REGISTER_BL_IMAGE_DESCS(s32g_bl2_mem_params_descs)
