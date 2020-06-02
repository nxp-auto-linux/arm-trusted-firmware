/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/bl_common.h>
#include <drivers/io/io_driver.h>
#include <drivers/mmc.h>
#include <drivers/nxp/s32g/io/io_mmc.h>
#include <drivers/io/io_fip.h>
#include <drivers/nxp/s32g/mmc/s32g274a_mmc.h>
#include <assert.h>
#include <tools_share/firmware_image_package.h>
#include "s32g_storage.h"

static const io_dev_connector_t *s32g_fip_io_dev;
static uintptr_t s32g_fip_dev_handle;

static const io_dev_connector_t *s32g_mmc_io_dev;
static uintptr_t s32g_mmc_dev_handle;

static int s32g_check_fip_dev(const uintptr_t spec);
static int s32g_check_mmc_dev(const uintptr_t spec);

static const io_block_spec_t fip_mmc_spec = {
	.offset = FIP_MMC_OFFSET,
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
};

static int s32g_check_fip_dev(const uintptr_t spec)
{
	int result;
	uintptr_t local_image_handle;

	result = io_dev_init(s32g_fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);
	if (!result) {
		result = io_open(s32g_fip_dev_handle, spec, &local_image_handle);
		if (!result)
			io_close(local_image_handle);
	}
	return result;
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

static void plat_s32g_io_setup(enum s32g_boot_source boot_source)
{
	uintptr_t handle;

	switch (boot_source) {
	case S32G_MMC_BOOT:
		handle = s32g_mmc_dev_handle;

		if (s32g274a_mmc_register())
			goto err_register;

		if (register_io_dev_mmc(&s32g_mmc_io_dev))
			goto err_register;

		if (io_dev_open(s32g_mmc_io_dev,
				(uintptr_t)&fip_mmc_spec,
				&s32g_mmc_dev_handle))
			goto err_io_dev_open;

		if (io_dev_init(s32g_mmc_dev_handle, 0))
			goto err_io_dev_init;

		break;

	case S32G_FIP_BOOT:
		handle = s32g_fip_dev_handle;

		if (register_io_dev_fip(&s32g_fip_io_dev))
			goto err_register;

		if (io_dev_open(s32g_fip_io_dev,
				(uintptr_t)&bl31_uuid_spec,
				&s32g_fip_dev_handle))
			goto err_io_dev_open;

		if (io_dev_init(s32g_fip_dev_handle, 0))
			goto err_io_dev_init;

		break;

	default:
		ERROR("Unknown boot source: %d", boot_source);
		goto err_boot_source;
	}

	return;

err_io_dev_init:
	io_dev_close(handle);
err_io_dev_open:
err_boot_source:
err_register:
	panic();
}

void s32g_io_setup(void)
{
	plat_s32g_io_setup(S32G_MMC_BOOT);
	plat_s32g_io_setup(S32G_FIP_BOOT);
}
