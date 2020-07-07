/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/bl_common.h>
#include <drivers/io/io_driver.h>
#include <drivers/mmc.h>
#include <drivers/nxp/s32g/io/io_mmc.h>
#include <drivers/io/io_memmap.h>
#include <drivers/io/io_fip.h>
#include <drivers/nxp/s32g/mmc/s32g_mmc.h>
#include <assert.h>
#include <tools_share/firmware_image_package.h>
#include "s32g_storage.h"

#define FIP_BACKEND_MEMMAP_ID	(BL33_IMAGE_ID + 1)

static const io_dev_connector_t *s32g_mmc_io_conn;
static uintptr_t s32g_mmc_dev_handle;

static const io_dev_connector_t *s32g_fip_io_conn;
static uintptr_t s32g_fip_dev_handle;

static const io_dev_connector_t *s32g_memmap_io_conn;
static uintptr_t s32g_memmap_dev_handle;

static int s32g_check_mmc_dev(const uintptr_t spec);
static int s32g_check_fip_dev(const uintptr_t spec);
static int s32g_check_memmap_dev(const uintptr_t spec);

static const io_block_spec_t fip_mmc_spec = {
	.offset = FIP_MMC_OFFSET,
	.length = ROUND_TO_MMC_BLOCK_SIZE(FIP_MAXIMUM_SIZE),
};

static const io_block_spec_t fip_memmap_spec = {
	.offset = FIP_BASE,
	.length = ROUND_TO_MMC_BLOCK_SIZE(FIP_MAXIMUM_SIZE),
};

static const io_uuid_spec_t bl31_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
};

static const io_uuid_spec_t bl33_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
};

static const struct plat_io_policy s32g_policies[] = {
	[FIP_IMAGE_ID] = {
		&s32g_mmc_dev_handle,
		(uintptr_t)&fip_mmc_spec,
		s32g_check_mmc_dev
	},
	[BL31_IMAGE_ID] = {
		&s32g_fip_dev_handle,
		(uintptr_t)&bl31_uuid_spec,
		s32g_check_fip_dev
	},
	[BL33_IMAGE_ID] = {
		&s32g_fip_dev_handle,
		(uintptr_t)&bl33_uuid_spec,
		s32g_check_fip_dev
	},
	[FIP_BACKEND_MEMMAP_ID] = {
		&s32g_memmap_dev_handle,
		(uintptr_t)&fip_memmap_spec,
		s32g_check_memmap_dev
	},
};

static int s32g_check_fip_dev(const uintptr_t spec)
{
	int ret;
	uintptr_t local_image_handle;

	ret = io_dev_init(s32g_fip_dev_handle,
			  (uintptr_t)FIP_BACKEND_MEMMAP_ID);
	if (ret)
		return ret;
	ret = io_open(s32g_fip_dev_handle, spec, &local_image_handle);
	if (ret)
		return ret;
	/* must be closed, as load_image() will do another io_open() */
	io_close(local_image_handle);

	return 0;
}

static int s32g_check_mmc_dev(const uintptr_t spec)
{
	uintptr_t local_handle;
	int ret;

	ret = io_open(s32g_mmc_dev_handle, spec, &local_handle);
	if (ret)
		return ret;
	/* must be closed, as load_image() will do another io_open() */
	io_close(local_handle);

	return 0;
}

static int s32g_check_memmap_dev(const uintptr_t spec)
{
	uintptr_t local_handle;
	int ret;

	return 0;

	ret = io_open(s32g_memmap_dev_handle, spec, &local_handle);
	if (ret)
		return ret;
	/* must be closed, as load_image() will do another io_open() */
	io_close(local_handle);

	return 0;
}

int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	const struct plat_io_policy *policy;
	int ret;

	assert(image_id < ARRAY_SIZE(s32g_policies));

	policy = &s32g_policies[image_id];
	assert(policy && policy->check);
	ret = policy->check(policy->image_spec);
	if (ret) {
		*dev_handle = (uintptr_t)NULL;
		*image_spec = (uintptr_t)NULL;
		return ret;
	}

	*dev_handle = *(policy->dev_handle);
	*image_spec = policy->image_spec;

	return 0;
}

void s32g_io_setup(void)
{
	if (s32g274a_mmc_register())
		goto err;
	if (register_io_dev_mmc(&s32g_mmc_io_conn))
		goto err;
	if (io_dev_open(s32g_mmc_io_conn, (uintptr_t)&fip_mmc_spec,
			&s32g_mmc_dev_handle))
		goto err;
	if (io_dev_init(s32g_mmc_dev_handle, 0))
		goto err;

	if (register_io_dev_fip(&s32g_fip_io_conn))
		goto err;
	if (io_dev_open(s32g_fip_io_conn, (uintptr_t)&bl31_uuid_spec,
			&s32g_fip_dev_handle))
		goto err;
	if (io_dev_init(s32g_fip_dev_handle, 0))
		goto err;

	if (register_io_dev_memmap(&s32g_memmap_io_conn))
		goto err;
	if (io_dev_open(s32g_memmap_io_conn, (uintptr_t)&fip_memmap_spec,
			&s32g_memmap_dev_handle))
		goto err;
	if (io_dev_init(s32g_memmap_dev_handle,
			(uintptr_t)FIP_BACKEND_MEMMAP_ID))
		goto err;

	return;
err:
	ERROR("Error: %s failed\n", __func__);
	panic();
}
