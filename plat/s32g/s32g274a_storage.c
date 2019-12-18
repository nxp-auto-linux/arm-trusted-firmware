/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/bl_common.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_memmap.h>
#include <assert.h>
#include <tools_share/firmware_image_package.h>
#include "s32g_storage.h"


static const io_dev_connector_t *s32g_sram_io_dev;
static uintptr_t s32g_sram_boot_dev_handle;

static int s32g_check_sram_dev(const uintptr_t spec);

static const io_block_spec_t bl31_sram_spec = {
	/* FIXME This layout is *only* valid for the development version */
	.offset = TEMP_S32G_BL31_READ_ADDR_IN_SRAM,
#if (TEMP_S32G_BL31_READ_ADDR_IN_SRAM < BL2_BASE)
#if (BL2_BASE - TEMP_S32G_BL31_READ_ADDR_IN_SRAM < BL31_LIMIT - BL31_BASE)
	.length = BL2_BASE - TEMP_S32G_BL31_READ_ADDR_IN_SRAM,
#else
	.length = BL31_LIMIT - BL31_BASE,
#endif
#else
#error "Unsupported BL31 layout, please check SRAM memory map"
#endif
};

static const struct plat_io_policy s32g_policies[] = {
	[BL31_IMAGE_ID] = {
		&s32g_sram_boot_dev_handle,
		(uintptr_t)&bl31_sram_spec,
		s32g_check_sram_dev
	},
};

static int s32g_check_sram_dev(const uintptr_t spec)
{
	uintptr_t local_handle;
	int ret;

	ret = io_open(s32g_sram_boot_dev_handle, spec, &local_handle);
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

	ret = register_io_dev_memmap(&s32g_sram_io_dev);
	if (ret)
		goto err_memmap;

	switch (boot_source) {
	case S32G_SRAM_BOOT:
		ret = io_dev_open(s32g_sram_io_dev,
				  (uintptr_t)&bl31_sram_spec,
				  &s32g_sram_boot_dev_handle);
		if (ret)
			goto err_io_dev_open;

		ret = io_dev_init(s32g_sram_boot_dev_handle,
				  (uintptr_t)BL31_IMAGE_ID);
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
	io_dev_close(s32g_sram_boot_dev_handle);
err_io_dev_open:
err_memmap:
	panic();
}

void s32g_io_setup(void)
{
	/* Assume next image is in SRAM at a known offset; while impractical in
	 * real-life deployment - mostly due to performance issues related to
	 * boot timing - this is convenient during development, when all images
	 * can be loaded by the BootROM.
	 */
	plat_s32g_io_setup(S32G_SRAM_BOOT);
}
