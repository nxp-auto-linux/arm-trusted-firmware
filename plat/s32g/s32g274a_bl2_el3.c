/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platform.h>
#include <common/bl_common.h>
#include <common/desc_image_load.h>
#include <common/debug.h>
#include <drivers/console.h>
#include <lib/mmio.h>
#include "s32g_ncore.h"
#include "s32g_pinctrl.h"
#include "s32g_clocks.h"
#include "s32g_linflexuart.h"
#include "s32g_storage.h"
#include "s32g_mc_rgm.h"
#include "s32g_mc_me.h"
#include <nxp/s32g/ddr/ddrss.h>
#include <drivers/generic_delay_timer.h>

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
	uint32_t caiutc;

	s32g_plat_config_pinctrl();
	s32g_plat_clock_init();

	/* Restore (clear) the CAIUTC[IsolEn] bit for the primay cluster, which
	 * we have manually set during early BL2 boot.
	 */
	caiutc = mmio_read_32(S32G_NCORE_CAIU0_BASE_ADDR + NCORE_CAIUTC_OFF);
	caiutc &= ~NCORE_CAIUTC_ISOLEN_MASK;
	mmio_write_32(S32G_NCORE_CAIU0_BASE_ADDR + NCORE_CAIUTC_OFF, caiutc);

	ncore_init();
	ncore_caiu_online(A53_CLUSTER0_CAIU);

	generic_delay_timer_init();

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

void bl2_el3_plat_arch_setup(void)
{
	static struct console_s32g console;
	extern struct ddrss_conf ddrss_conf;
	extern struct ddrss_firmware ddrss_firmware;
	extern void sram_clr(uintptr_t start, size_t end);

	console_s32g_register(S32G_UART_BASE, S32G_UART_CLOCK_HZ,
			      S32G_UART_BAUDRATE, &console);

	if (get_reset_cause() == CAUSE_WAKEUP_DURING_STANDBY) {
		ddrss_to_normal_mode(&ddrss_conf, &ddrss_firmware);
		return;
	}

	sram_clr(STANDBY_SRAM_BASE, STANDBY_SRAM_USED_FOR_CSR);
	ddrss_init(&ddrss_conf, &ddrss_firmware);
}

REGISTER_BL_IMAGE_DESCS(s32g_bl2_mem_params_descs)
