/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <s32_bl_common.h>
#include <fconf/s32n_fconf_ufs_getter.h>
#include <drivers/dw_ufs.h>
#include <drivers/nxp/s32n/s32n_ufs.h>
#include <drivers/ufs.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

#define UFS_DESC_SIZE	0x400

/* The structure pointed to by UTRLBA is 1KB in length and shall
 * be 1KB aligned.
 */
static uint8_t ufs_desc[UFS_DESC_SIZE] __aligned(UFS_DESC_SIZE);

static int s32n_phy_init(ufs_params_t *params)
{
	return 0;
}

static const ufs_ops_t s32n_ufs_ops = {
	.phy_init = s32n_phy_init,
	.phy_set_pwr_mode = dwufs_phy_set_pwr_mode,
};

int s32n_ufs_init(void)
{
	ufs_params_t ufs_params = {
		.desc_base = (uintptr_t)&ufs_desc,
		.desc_size = sizeof(ufs_desc),
		.flags = 0,
	};
	size_t size;
	int ret;

	ufs_params.reg_base = FCONF_GET_PROPERTY(hw_config, ufs_config,
						 reg_addr);
	size = FCONF_GET_PROPERTY(hw_config, ufs_config,
						 reg_size);

	if (!ufs_params.reg_base || !size) {
		ERROR("No UFS node found enabled in the device tree\n");
		panic();
	}

	ret = mmap_add_dynamic_region(ufs_params.reg_base,
			ufs_params.reg_base, MMU_ROUND_UP_TO_PAGE(size),
			MT_DEVICE | MT_RW);

	if (ret < 0) {
		ERROR("DTB mapping: error %d\n", ret);
		return ret;
	}

	ret = ufs_init(&s32n_ufs_ops, &ufs_params);
	if (ret) {
		ERROR("The UFS initialization failed\n");
		return ret;
	}

	return 0;
}

