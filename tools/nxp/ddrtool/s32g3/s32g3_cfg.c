/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma redefine_extname ddrc_cfg s32g3_ddrc_cfg
#pragma redefine_extname ddrc_cfg_size s32g3_ddrc_cfg_size
#pragma redefine_extname dmem_1d_cfg s32g3_dmem_1d_cfg
#pragma redefine_extname dmem_1d_cfg_size s32g3_dmem_1d_cfg_size
#pragma redefine_extname dmem_2d_cfg s32g3_dmem_2d_cfg
#pragma redefine_extname dmem_2d_cfg_size s32g3_dmem_2d_cfg_size
#pragma redefine_extname dq_swap_cfg s32g3_dq_swap_cfg
#pragma redefine_extname dq_swap_cfg_size s32g3_dq_swap_cfg_size
#pragma redefine_extname imem_1d_cfg s32g3_imem_1d_cfg
#pragma redefine_extname imem_1d_cfg_size s32g3_imem_1d_cfg_size
#pragma redefine_extname imem_2d_cfg s32g3_imem_2d_cfg
#pragma redefine_extname imem_2d_cfg_size s32g3_imem_2d_cfg_size
#pragma redefine_extname phy_cfg s32g3_phy_cfg
#pragma redefine_extname phy_cfg_size s32g3_phy_cfg_size
#pragma redefine_extname pie_cfg s32g3_pie_cfg
#pragma redefine_extname pie_cfg_size s32g3_pie_cfg_size
#pragma redefine_extname csr_to_store s32g3_csr_to_store
#pragma redefine_extname csr_to_store_size s32g3_csr_to_store_size
#pragma redefine_extname ddrc_to_store s32g3_ddrc_to_store
#pragma redefine_extname ddrc_to_store_size s32g3_ddrc_to_store_size

#include "s32g3/ddrc_cfg.c"
#include "s32g3/dmem_cfg.c"
#include "s32g3/dq_swap_cfg.c"
#include "s32g3/phy_cfg.c"
#include "s32g3/pie_cfg.c"
#include "imem_cfg.c"
#include "ddr_lp_csr.c"
#include "../ddrtool.h"

void s32g3_populate_ddr_conf(ddr_fw_t *ddr_fw, ddr_params_t *params)
{
	const ddr_fw_t priv_ddr_fw = S32CC_DDR_FW_INIT();

	*ddr_fw = priv_ddr_fw;

	if (params->enable_ecc) {
		ddr_fw->ddrc_cfg[15].data = 0x033f7f54U;
		ddr_fw->ddrc_cfg[16].data = 0x00000780U;
		ddr_fw->ddrc_cfg[63].data = 0x00000014U;
		ddr_fw->ddrc_cfg[64].data = 0x00050505U;
		ddr_fw->ddrc_cfg[66].data = 0x14141400U;
		ddr_fw->ddrc_cfg[68].data = 0x04040404U;
		ddr_fw->ddrc_cfg[69].data = 0x04040404U;
		ddr_fw->ddrc_cfg[121].data = 0x37ffffffU;
	}
	if (params->enable_perf) {
		ddr_fw->ddrc_cfg[77].data = 0x00001f01U;
		ddr_fw->ddrc_cfg[80].data = 0x90000096U;
		ddr_fw->ddrc_cfg[81].data = 0x1000012cU;
	}
}
