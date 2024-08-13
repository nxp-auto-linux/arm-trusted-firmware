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
#include <drivers/mmc.h>
#include <drivers/nxp/s32/mmc/s32_mmc.h>
#include <drivers/nxp/s32n/s32n_ufs.h>
#include <drivers/partition/partition.h>
#include <drivers/ufs.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <s32_bl_common.h>
#include <tools_share/firmware_image_package.h>

#include <platform_def.h>

#define S32N_LUN0			0

enum {
	BOOT_SOURCE_MEM = 0,
	BOOT_SOURCE_UFS = 1,
	BOOT_SOURCE_MMC = 2,
};

struct plat_io_policy {
	uintptr_t *dev_handle;
	uintptr_t image_spec;
	int (*check)(const uintptr_t spec);
};

static const io_dev_connector_t *memmap_dev_con;
static const io_dev_connector_t *ufs_dev_con;
static const io_dev_connector_t *mmc_dev_con;

static uintptr_t boot_dev_handle;
static uintptr_t fip_dev_handle;
static const io_dev_connector_t *fip_dev_con;

static const io_block_spec_t mem_fip_spec = {
	.offset = S32N_FIP_BASE,
	.length = S32N_FIP_SIZE,
};

static io_block_spec_t ufs_fip_spec = {
	.offset = 0x0,
	.length = S32N_IO_BUFFER_SIZE,
};

static io_block_spec_t mmc_fip_spec = {
	.offset = 0x0,
	.length = S32N_IO_BUFFER_SIZE,
};

/* Each partition entry occupies 128B. */
#define PARTITION_ENTRIES	(PLAT_PARTITION_BLOCK_SIZE / \
				128 * PLAT_PARTITION_MAX_ENTRIES)
static const io_block_spec_t gpt_block_spec = {
	.offset = 0,
	.length	= PLAT_PARTITION_BLOCK_SIZE * (PARTITION_ENTRIES + 2),
};

size_t ufs_read_blks(int lba, uintptr_t buf, size_t size)
{
	return ufs_read_blocks(S32N_LUN0, lba, buf, size);
}

static const io_block_dev_spec_t ufs_dev_spec = {
	/* It's used as temp buffer in block driver. */
	.buffer		= {
		.offset	= S32N_IO_BUFFER_BASE,
		.length	= S32N_IO_BUFFER_SIZE,
	},
	.ops		= {
		.read	= ufs_read_blks,
	},
	.block_size	= UFS_BLOCK_SIZE,
};

static const io_block_dev_spec_t mmc_dev_spec = {
	/* It's used as temp buffer in block driver. */
	.buffer		= {
		.offset	= S32N_IO_BUFFER_BASE,
		.length	= S32N_IO_BUFFER_SIZE,
	},
	.ops		= {
		.read	= mmc_read_blocks,
	},
	.block_size	= MMC_BLOCK_SIZE,
};

static const io_uuid_spec_t bl31_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
};

static const io_uuid_spec_t bl33_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
};

static int check_dev(const uintptr_t spec)
{
	uintptr_t img_handle;
	int result;

	result = io_dev_init(boot_dev_handle, (uintptr_t)NULL);
	if (result)
		return result;

	result = io_open(boot_dev_handle, spec, &img_handle);
	if (result == 0)
		(void)io_close(img_handle);

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

static struct plat_io_policy policies[] = {
	[FIP_IMAGE_ID] = {
		.dev_handle = &boot_dev_handle,
		.check = check_dev,
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
	[GPT_IMAGE_ID] = {
		.dev_handle = &boot_dev_handle,
		.image_spec = (uintptr_t)&gpt_block_spec,
		.check = check_dev,
	},
};

static inline int boot_source_get(void)
{
	if (S32N_DDR_BOOT_SOURCE)
		return BOOT_SOURCE_MEM;
	if (S32N_UFS_BOOT_SOURCE)
		return BOOT_SOURCE_UFS;
	if (S32N_MMC_BOOT_SOURCE)
		return BOOT_SOURCE_MMC;
	return -EINVAL;
}

static void plat_s32n_mem_setup(void)
{
	int result;

	result = register_io_dev_memmap(&memmap_dev_con);
	assert(result == 0);

	result = io_dev_open(memmap_dev_con, (uintptr_t)NULL,
			&boot_dev_handle);
	assert(result == 0);

	result = mmap_add_dynamic_region(S32N_FIP_BASE, S32N_FIP_BASE,
			MMU_ROUND_UP_TO_PAGE(S32N_FIP_SIZE),
			MT_MEMORY | MT_RW | MT_SECURE);
	if (result < 0) {
		ERROR("FIP mapping: error %d\n", result);
		panic();
	}
}

static void plat_s32n_ufs_setup(void)
{
	int result;

	result = s32n_ufs_init();
	if (result)
		panic();

	result = mmap_add_dynamic_region(S32N_IO_BUFFER_BASE,
				      S32N_IO_BUFFER_BASE,
				      MMU_ROUND_UP_TO_PAGE(S32N_IO_BUFFER_SIZE),
				      MT_MEMORY | MT_RW | MT_SECURE);
	if (result < 0) {
		ERROR("UFS temporary buffer mapping: error %d\n", result);
		panic();
	}

	result = register_io_dev_block(&ufs_dev_con);
	assert(result == 0);

	result = io_dev_open(ufs_dev_con, (uintptr_t)&ufs_dev_spec,
			     &boot_dev_handle);
	assert(result == 0);

	/* The FIP image is located in the "FIP" partion. */
	partition_init(GPT_IMAGE_ID);
	ufs_fip_spec.offset = get_partition_entry("FIP")->start;
}

static void plat_s32n_mmc_setup(void)
{
	int result;

	result = s32_mmc_register();
	if (result)
		panic();

	result = mmap_add_dynamic_region(S32N_IO_BUFFER_BASE,
				      S32N_IO_BUFFER_BASE,
				      MMU_ROUND_UP_TO_PAGE(S32N_IO_BUFFER_SIZE),
				      MT_MEMORY | MT_RW | MT_SECURE);
	if (result < 0) {
		ERROR("UFS temporary buffer mapping: error %d\n", result);
		panic();
	}

	result = register_io_dev_block(&mmc_dev_con);
	assert(result == 0);

	result = io_dev_open(mmc_dev_con, (uintptr_t)&mmc_dev_spec,
			     &boot_dev_handle);
	assert(result == 0);
}

void plat_s32n_io_setup(void)
{
	int boot_source = boot_source_get();
	int result __unused;

	if (boot_source < 0) {
		ERROR("Invalid boot source.\n");
		panic();
	}

	switch (boot_source) {
	case BOOT_SOURCE_UFS:
		INFO("Booting from UFS\n");
		policies[FIP_IMAGE_ID].image_spec = (uintptr_t)&ufs_fip_spec;
		plat_s32n_ufs_setup();
		break;
	case BOOT_SOURCE_MMC:
		INFO("Booting from eMMC\n");
		policies[FIP_IMAGE_ID].image_spec = (uintptr_t)&mmc_fip_spec;
		plat_s32n_mmc_setup();
		break;
	case BOOT_SOURCE_MEM:
		INFO("Booting from DDR\n");
		policies[FIP_IMAGE_ID].image_spec = (uintptr_t)&mem_fip_spec;
		plat_s32n_mem_setup();
	default:
		break;
	}

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
