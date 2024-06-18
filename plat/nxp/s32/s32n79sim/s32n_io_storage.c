/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <stdint.h>
#include <drivers/io/io_block.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_fip.h>
#include <drivers/io/io_memmap.h>
#include <drivers/io/io_storage.h>
#include <drivers/ufs.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <s32_bl_common.h>
#include <tools_share/firmware_image_package.h>

#include <platform_def.h>

#define S32N_LUN0			0

struct plat_io_policy {
	uintptr_t *dev_handle;
	uintptr_t image_spec;
	int (*check)(const uintptr_t spec);
};

static uintptr_t ufs_dev_handle;
static const io_dev_connector_t *ufs_dev_con;

static uintptr_t fip_dev_handle;
static const io_dev_connector_t *fip_dev_con;

static io_block_spec_t ufs_fip_spec = {
	.offset = 0x0,
	.length = S32N_UFS_DATA_SIZE,
};

size_t ufs_read_blks(int lba, uintptr_t buf, size_t size)
{
	return ufs_read_blocks(S32N_LUN0, lba, buf, size);
}

static const io_block_dev_spec_t ufs_dev_spec = {
	/* It's used as temp buffer in block driver. */
	.buffer		= {
		.offset	= S32N_UFS_DATA_BASE,
		.length	= S32N_UFS_DATA_SIZE,
	},
	.ops		= {
		.read	= ufs_read_blks,
	},
	.block_size	= UFS_BLOCK_SIZE,
};

static const io_uuid_spec_t bl31_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
};

static const io_uuid_spec_t bl33_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
};

static int check_ufs(const uintptr_t spec)
{
	uintptr_t img_handle;
	int result;

	result = io_dev_init(ufs_dev_handle, (uintptr_t)NULL);
	if (result)
		return result;

	result = io_open(ufs_dev_handle, spec, &img_handle);
	if (result == 0)
		io_close(img_handle);

	return result;
}

static int check_fip(const uintptr_t spec)
{
	uintptr_t img_handle;
	int result;

	result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);
	if (result)
		return result;

	result = io_open(fip_dev_handle, spec, &img_handle);
	if (result == 0)
		(void)io_close(img_handle);

	return result;
}

static const struct plat_io_policy policies[] = {
	[FIP_IMAGE_ID] = {
		.dev_handle = &ufs_dev_handle,
		.image_spec = (uintptr_t)&ufs_fip_spec,
		.check = check_ufs,
	},
	[BL31_IMAGE_ID] = {
		.dev_handle = &fip_dev_handle,
		.image_spec = (uintptr_t)&bl31_uuid_spec,
		.check = check_fip,
	},
	[BL33_IMAGE_ID] = {
		.dev_handle = &fip_dev_handle,
		.image_spec = (uintptr_t)&bl33_uuid_spec,
		.check = check_fip,
	},
};

void plat_s32n_io_setup(void)
{
	int result __unused;

	result = mmap_add_dynamic_region(S32N_UFS_DATA_BASE,
				      S32N_UFS_DATA_BASE,
				      MMU_ROUND_UP_TO_PAGE(S32N_UFS_DATA_SIZE),
				      MT_MEMORY | MT_RW | MT_SECURE);
	if (result < 0) {
		ERROR("UFS temporary buffer mapping: error %d\n", result);
		panic();
	}

	result = register_io_dev_block(&ufs_dev_con);
	assert(result == 0);

	result = io_dev_open(ufs_dev_con, (uintptr_t)&ufs_dev_spec,
			     &ufs_dev_handle);
	assert(result == 0);

	result = register_io_dev_fip(&fip_dev_con);
	assert(result == 0);

	result = io_dev_open(fip_dev_con, (uintptr_t)NULL, &fip_dev_handle);
	assert(result == 0);
}

int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	const struct plat_io_policy *policy;
	int result __unused;

	assert(image_id < ARRAY_SIZE(policies));

	policy = &policies[image_id];
	result = policy->check(policy->image_spec);
	assert(result == 0);

	*image_spec = policy->image_spec;
	*dev_handle = *policy->dev_handle;

	return 0;
}
