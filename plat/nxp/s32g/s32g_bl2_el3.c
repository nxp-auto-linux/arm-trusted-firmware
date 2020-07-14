/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform.h>
#include <common/bl_common.h>
#include <common/desc_image_load.h>
#include <common/debug.h>
#include <drivers/console.h>
#include <lib/mmio.h>
#include "s32g_linflexuart.h"
#include "s32g_storage.h"
#include "s32g_mc_rgm.h"
#include "s32g_mc_me.h"
#include "bl31_ssram.h"
#include "s32g_lowlevel.h"
#include "s32g_bl_common.h"
#include <nxp/s32g/ddr/ddrss.h>
#include <drivers/generic_delay_timer.h>
#include <ssram_mailbox.h>

static bl_mem_params_node_t s32g_bl2_mem_params_descs[] = {
	{
		.image_id = FIP_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      NON_SECURE | EXECUTABLE),

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, IMAGE_ATTRIB_PLAT_SETUP),
		.image_info.image_max_size = FIP_MAXIMUM_SIZE,
		.image_info.image_base = FIP_BASE,
		.next_handoff_image_id = BL31_IMAGE_ID,
	},
	{
		.image_id = BL31_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      SECURE | EXECUTABLE | EP_FIRST_EXE),
		.ep_info.spsr = SPSR_64(MODE_EL3, MODE_SP_ELX,
					DISABLE_ALL_EXCEPTIONS),
		.ep_info.pc = BL31_BASE,

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, 0),
		.image_info.image_max_size = BL31_LIMIT - BL31_BASE,
		.image_info.image_base = BL31_BASE,
		.next_handoff_image_id = BL33_IMAGE_ID,
	},
	{
		.image_id = BL33_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      NON_SECURE | EXECUTABLE),

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, 0),
		.image_info.image_max_size = S32G_BL33_IMAGE_SIZE,
		.image_info.image_base = S32G_BL33_IMAGE_BASE,
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
	s32g_early_plat_init(false);
	s32g_io_setup();
}

enum reset_cause get_reset_cause(void)
{
	uint32_t mc_rgm_des = mmio_read_32(MC_RGM_DES);

	if (mc_rgm_des & DES_F_POR)
		return CAUSE_POR;

	if (mc_rgm_des & DES_F_DR_ANY) {
		if (mmio_read_32(MC_RGM_RDSS) & RDSS_DES_RES)
			return CAUSE_DESTRUCTIVE_RESET_DURING_STANDBY;
		else
			return CAUSE_DESTRUCTIVE_RESET_DURING_RUN;
	}

	if (mmio_read_32(MC_RGM_FES) & FES_F_FR_ANY) {
		if (mmio_read_32(MC_RGM_RDSS) & RDSS_FES_RES)
			return CAUSE_FUNCTIONAL_RESET_DURING_STANDBY;
		else
			return CAUSE_FUNCTIONAL_RESET_DURING_RUN;
	}

	if (mmio_read_32(MC_ME_MODE_STAT) & MODE_STAT_PREV_MODE)
		return CAUSE_WAKEUP_DURING_STANDBY;

	return CAUSE_ERROR;
}

static void copy_bl31ssram_image(void)
{
	/* Copy bl31 ssram stage. This includes IVT */
	memcpy((void *)S32G_SSRAM_BASE, bl31ssram, bl31ssram_len);
}

void bl2_el3_plat_arch_setup(void)
{
	static struct console_s32g console;
	void sram_clr(uintptr_t start, size_t size);
	extern struct ddrss_conf ddrss_conf;
	extern struct ddrss_firmware ddrss_firmware;

	console_s32g_register(S32G_UART_BASE, S32G_UART_CLOCK_HZ,
			      S32G_UART_BAUDRATE, &console);

	sram_clr(S32G_BL33_IMAGE_BASE, BL2_BASE - S32G_BL33_IMAGE_BASE);

	sram_clr(S32G_SSRAM_BASE, S32G_SSRAM_LIMIT - S32G_SSRAM_BASE);
	copy_bl31ssram_image();
	/* This will also populate CSR section from bl31ssram */
	ddrss_init(&ddrss_conf, &ddrss_firmware, BL31SSRAM_CSR_BASE);
}

REGISTER_BL_IMAGE_DESCS(s32g_bl2_mem_params_descs)
