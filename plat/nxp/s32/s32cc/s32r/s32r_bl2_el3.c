/*
 * Copyright 2021-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/desc_image_load.h>
#include "ddr_init.h"
#include <lib/libc/errno.h>
#include <lib/mmio.h>
#include "platform_def.h"
#include "s32cc_bl_common.h"
#include "s32cc_bl2_el3.h"
#include "s32cc_linflexuart.h"
#include "s32cc_storage.h"
#include "s32cc_sramc.h"

static bl_mem_params_node_t s32r_bl2_mem_params_descs[7];
REGISTER_BL_IMAGE_DESCS(s32r_bl2_mem_params_descs)
static enum reset_cause reset_cause;

enum reset_cause get_reset_cause(void)
{
	return reset_cause;
}

static enum reset_cause read_reset_cause(void)
{
	uint32_t mc_rgm_des = mmio_read_32(MC_RGM_DES);
	uint32_t mc_rgm_fes = mmio_read_32(MC_RGM_FES);

	if (mc_rgm_des & DES_F_POR)
		return CAUSE_POR;

	if (mc_rgm_des & DES_F_DR_ANY)
		return CAUSE_DESTRUCTIVE_RESET_DURING_RUN;

	if (mc_rgm_fes & FES_F_EXR)
		return CAUSE_EXTERNAL_RESET_DURING_RUN;

	if (mc_rgm_fes & FES_F_FR_ANY)
		return CAUSE_FUNCTIONAL_RESET_DURING_RUN;

	return CAUSE_ERROR;
}

void clear_reset_cause(void)
{
	mmio_setbits_32(MC_RGM_DES, 0);
	mmio_setbits_32(MC_RGM_FES, 0);
}

void bl2_el3_early_platform_setup(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	size_t index = 0;
	bl_mem_params_node_t *params = s32r_bl2_mem_params_descs;
	size_t params_size = ARRAY_SIZE(s32r_bl2_mem_params_descs);
	int ret = 0;

	reset_cause = CAUSE_POR;
	if (s32cc_has_reset_access()) {
		reset_cause = read_reset_cause();
		clear_reset_cause();
		set_func_reset_threshold();
	}

	s32_early_plat_init();

	wait_hse_init();

	s32_early_plat_setup();

	ret |= add_ddr_fw_img_to_mem_params_descs(params, &index, params_size);
	ret |= add_bl31_img_to_mem_params_descs(params, &index, params_size);
	ret |= add_bl32_img_to_mem_params_descs(params, &index, params_size);
	ret |= add_bl32_extra1_img_to_mem_params_descs(params, &index,
						      params_size);
	ret |= add_bl33_img_to_mem_params_descs(params, &index, params_size);
	if (ret)
		panic();

	bl_mem_params_desc_num = index;
}
