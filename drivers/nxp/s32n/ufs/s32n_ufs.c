/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <drivers/dw_ufs.h>
#include <drivers/nxp/s32n/s32n_ufs.h>
#include <drivers/ufs.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

#define UFSHS_BASE 0x4ac80000
#define UFSHS_SIZE 0x1000
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
		.reg_base = UFSHS_BASE,
		.desc_base = (uintptr_t)&ufs_desc,
		.desc_size = sizeof(ufs_desc),
		.flags = 0,
	};
	int ret;

	ret = mmap_add_dynamic_region(UFSHS_BASE, UFSHS_BASE,
				      UFSHS_SIZE,
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

