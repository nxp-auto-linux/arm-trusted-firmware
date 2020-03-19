/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/bl_common.h>
#include <drivers/io/io_driver.h>
#include <drivers/nxp/s32g/io/io_mmc.h>
#include <drivers/nxp/s32g/mmc/s32g274a_mmc.h>
#include <assert.h>
#include <tools_share/firmware_image_package.h>
#include "s32g_storage.h"

static const io_dev_connector_t *s32g_mmc_io_dev;
static uintptr_t s32g_mmc_boot_dev_handle;

static int s32g_check_mmc_dev(const uintptr_t spec);

static const io_block_spec_t bl31_mmc_spec = {
	.offset = BL31_MMC_OFFSET,
	.length = BL31_MMC_SIZE,
};

static const io_block_spec_t bl33_mmc_spec = {
	.offset = BL33_MMC_OFFSET,
	.length = BL33_MMC_SIZE,
};

static const struct plat_io_policy s32g_policies[] = {
	[BL31_IMAGE_ID] = {
		&s32g_mmc_boot_dev_handle,
		(uintptr_t)&bl31_mmc_spec,
		s32g_check_mmc_dev
	},
	[BL33_IMAGE_ID] = {
		&s32g_mmc_boot_dev_handle,
		(uintptr_t)&bl33_mmc_spec,
		s32g_check_mmc_dev
	},
};

static int s32g_check_mmc_dev(const uintptr_t spec)
{
	uintptr_t local_handle;
	int ret;

	ret = io_open(s32g_mmc_boot_dev_handle, spec, &local_handle);
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
	int ret;

	ret = s32g274a_mmc_register();
	if (ret)
		goto err_register;

	ret = register_io_dev_mmc(&s32g_mmc_io_dev);
	if (ret)
		goto err_register;

	switch (boot_source) {
	case S32G_MMC_BOOT:
		ret = io_dev_open(s32g_mmc_io_dev,
				  (uintptr_t)&bl31_mmc_spec,
				  &s32g_mmc_boot_dev_handle);
		if (ret)
			goto err_io_dev_open;

		ret = io_dev_init(s32g_mmc_boot_dev_handle, 0);
		if (ret)
			goto err_io_dev_init;

		break;
	default:
		ERROR("Unknown boot source: %d", boot_source);
		goto err_boot_source;
	}

	return;

err_boot_source:
err_io_dev_init:
	io_dev_close(s32g_mmc_boot_dev_handle);
err_io_dev_open:
err_register:
	panic();
}

void s32g_io_setup(void)
{
	plat_s32g_io_setup(S32G_MMC_BOOT);
}
