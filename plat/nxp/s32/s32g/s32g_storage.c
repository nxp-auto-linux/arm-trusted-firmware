/*
 * Copyright 2019-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/bl_common.h>
#include <common/desc_image_load.h>
#include <drivers/io/io_driver.h>
#include <drivers/mmc.h>
#include <drivers/nxp/s32g/io/io_mmc.h>
#include <drivers/io/io_memmap.h>
#include <drivers/io/io_fip.h>
#include <drivers/nxp/s32g/mmc/s32g_mmc.h>
#include <assert.h>
#include <tools_share/firmware_image_package.h>
#include <lib/mmio.h>
#include <libfdt.h>
#include <common/fdt_wrappers.h>

#include "s32g_storage.h"
#include "s32g_bl_common.h"
#include "s32g_dt.h"

#define FIP_BACKEND_MEMMAP_ID	(BL33_IMAGE_ID + 1)

#define EEPROM_CHIP_ADDR		0x50
#define EEPROM_BOOT_CFG_OFF		0x0
#define EEPROM_ADDR_LEN			1

static const io_dev_connector_t *s32g_mmc_io_conn;
static uintptr_t s32g_mmc_dev_handle;

static const io_dev_connector_t *s32g_memmap_io_conn;
static uintptr_t s32g_memmap_dev_handle;

static int s32g_check_mmc_dev(const uintptr_t spec);
static int s32g_check_memmap_dev(const uintptr_t spec);

static const io_block_spec_t fip_memmap_spec = {
	.offset = FIP_BASE,
	.length = ROUND_TO_MMC_BLOCK_SIZE(FIP_HEADER_SIZE),
};

struct image_storage_info {
	uuid_t uuid;
	unsigned int image_id;
	io_block_spec_t mmc_spec;
	io_block_spec_t qspi_spec;
#ifdef FIP_MEM_OFFSET
	io_block_spec_t mem_spec;
#endif
};

static struct image_storage_info images_info[] = {
	{
		.image_id = FIP_IMAGE_ID,
		/* The selection of mmc, qspi, or mem spec is done dynamically,
		 * based on the boot source and config (e.g. FIP_MEM_OFFSET)
		 * */
		.mmc_spec = {
			.offset = FIP_MMC_OFFSET,
			.length = FIP_HEADER_SIZE,
		},
		.qspi_spec = {
			.offset = FIP_QSPI_OFFSET,
			.length = FIP_HEADER_SIZE,
		},
#ifdef FIP_MEM_OFFSET
		.mem_spec = {
			.offset = FIP_MEM_OFFSET,
			.length = FIP_HEADER_SIZE,
		},
#endif
	},
	{
		.image_id = BL31_IMAGE_ID,
		.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,

		/* The offset and length for the specs will be dynamically
		 * adjusted after reading FIP header. This is done after
		 * reading FIP header in SRAM, before loading any other
		 * image from FIP, by calling set_image_spec() function
		 * */
	},
#ifdef SPD_opteed
	{
		.image_id = BL32_IMAGE_ID,
		.uuid = UUID_SECURE_PAYLOAD_BL32,
	},
	{
		.image_id = BL32_EXTRA1_IMAGE_ID,
		.uuid = UUID_SECURE_PAYLOAD_BL32_EXTRA1,
	},
#endif
	{
		.image_id = BL33_IMAGE_ID,
		.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
	},
};

static struct plat_io_policy s32g_policies[] = {
	[FIP_BACKEND_MEMMAP_ID] = {
		&s32g_memmap_dev_handle,
		(uintptr_t)&fip_memmap_spec,
		s32g_check_memmap_dev
	},
};

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

static uint8_t eeprom_boot_source(void)
{
	void *fdt;
	const char *path;
	int i2c_node, ret;
	struct s32g_i2c_driver *driver;
	uint8_t boot_source;

	ret = dt_open_and_check();
	if (ret < 0)
		goto eeprom_boot_src_err;

	if (fdt_get_address(&fdt) == 0)
		goto eeprom_boot_src_err;

	path = fdt_get_alias(fdt, "i2c0");
	if (path == NULL) {
		INFO("No i2c0 property in aliases node\n");
		goto eeprom_boot_src_err;
	}

	i2c_node = fdt_path_offset(fdt, path);
	if (i2c_node < 0) {
		INFO("Failed to locate i2c0 node using its path\n");
		goto eeprom_boot_src_err;
	}

	driver = s32g_add_i2c_module(fdt, i2c_node);
	if (driver ==  NULL) {
		NOTICE("Failed to register i2c0 instance!\n");
		goto eeprom_boot_src_err;
	}

	s32g_i2c_read(&driver->bus, EEPROM_CHIP_ADDR, EEPROM_BOOT_CFG_OFF,
					EEPROM_ADDR_LEN, &boot_source, 1);
	boot_source = boot_source >> BOOT_SOURCE_OFF;

	return boot_source;

eeprom_boot_src_err:
	return INVALID_BOOT_SOURCE;
}

static uint8_t get_boot_source(void)
{
	uint32_t boot_cfg;
	static uint8_t boot_source = INVALID_BOOT_SOURCE;

	if (boot_source != INVALID_BOOT_SOURCE)
		return boot_source;

	boot_cfg = mmio_read_32(BOOT_GPR_BASE + BOOT_GPR_BMR1_OFF);

	if (boot_cfg & BOOT_RCON_MODE_MASK)
		boot_source = eeprom_boot_source();
	else
		boot_source = (boot_cfg & BOOT_SOURCE_MASK) >> BOOT_SOURCE_OFF;

	switch (boot_source) {

	case BOOT_SOURCE_QSPI:
	case BOOT_SOURCE_SD:
	case BOOT_SOURCE_MMC:
		return boot_source;

	default:
		ERROR("Could not identify the boot source\n");
		return INVALID_BOOT_SOURCE;
	}
}

static bool is_mmc_boot_source()
{
	uint8_t boot_source = get_boot_source();

#ifdef FIP_MEM_OFFSET
	return false;
#endif

	if ((boot_source == BOOT_SOURCE_SD) ||
	    (boot_source == BOOT_SOURCE_MMC))
		return true;

	return false;
}

static io_block_spec_t *get_image_spec_source(struct image_storage_info *info)
{
	uint8_t boot_source = get_boot_source();

	if (info == NULL)
		return NULL;

#ifdef FIP_MEM_OFFSET
	return &info->mem_spec;
#endif

	switch (boot_source) {
		case BOOT_SOURCE_QSPI:
			return &info->qspi_spec;
		case BOOT_SOURCE_SD:
		case BOOT_SOURCE_MMC:
			return &info->mmc_spec;
		default:
			return NULL;
	}
}

static io_block_spec_t * get_image_spec_from_id(unsigned int image_id)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(images_info); i++) {
		if (images_info[i].image_id == image_id)
			return get_image_spec_source(&images_info[i]);
	}

	return NULL;
}

static io_block_spec_t * get_image_spec_from_uuid(const uuid_t *uuid)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(images_info); i++) {
		if (compare_uuids(&images_info[i].uuid, uuid) == 0)
			return get_image_spec_source(&images_info[i]);
	}

	return NULL;
}

/* This function is called after reading the FIP header and before loading
 * any other image from FIP.
 * The size and offset parameters are read from the FIP header
 * */
void set_image_spec(const uuid_t *uuid, uint64_t size, uint64_t offset)
{
	io_block_spec_t *spec = get_image_spec_from_uuid(uuid);
	io_block_spec_t *fip_spec = get_image_spec_from_id(FIP_IMAGE_ID);

	if (spec == NULL)
		return;

	if (fip_spec == NULL) {
		ERROR("Invalid FIP io block spec\n");
		return;
	}

	if (is_mmc_boot_source())
		spec->length = ROUND_TO_MMC_BLOCK_SIZE(size);
	else
		spec->length = size;

	/* In FIP header the offset is relative to the FIP header start.
	 * The real offset of the image is computed by adding the offset
	 * from the FIP header to the real FIP offset
	 * */
	spec->offset = fip_spec->offset + offset;
}

/* Before loading each image (e.g. load_image), this function is called from
 * plat_get_image_source() and performs the following actions:
 * - For FIP_IMAGE_ID it sets the io_block spec source: mmc, qspi, or memory.
 *   The offset and length are statically initialized and always only the FIP
 *   header is read.
 * - For the other images in FIP (e.g. BL31, BL32, BL33) this function is
 *   is called after the FIP header is parsed and the right offset and length
 *   for each image in the FIP header are set.
 *   When the function is called, it just passes the spec source (mmc, qspi or
 *   mem) that should have been already updated.
 * */
static void set_img_source(struct plat_io_policy *policy,
			       unsigned int image_id)
{
	io_block_spec_t *crt_spec = get_image_spec_from_id(image_id);

	if (crt_spec == NULL)
		return;

	policy->image_spec = (uintptr_t)crt_spec;

	if (is_mmc_boot_source()) {
		policy->dev_handle = &s32g_mmc_dev_handle;
		policy->check = s32g_check_mmc_dev;
	} else {
		policy->dev_handle = &s32g_memmap_dev_handle;
		policy->check = s32g_check_memmap_dev;
	}
}


int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	const struct plat_io_policy *policy;
	int ret;

	assert(image_id < ARRAY_SIZE(s32g_policies));

	set_img_source(&s32g_policies[image_id], image_id);

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
	uint8_t boot_source;

	if (register_io_dev_memmap(&s32g_memmap_io_conn))
		goto err;
	if (io_dev_open(s32g_memmap_io_conn, (uintptr_t)&fip_memmap_spec,
			&s32g_memmap_dev_handle))
		goto err;
	if (io_dev_init(s32g_memmap_dev_handle,
			(uintptr_t)FIP_BACKEND_MEMMAP_ID))
		goto err;

	boot_source = get_boot_source();
	if (boot_source == INVALID_BOOT_SOURCE)
		goto err;

	/* MMC/SD may not be inserted */
	if (boot_source != BOOT_SOURCE_QSPI) {
		if (s32g_mmc_register(boot_source))
			goto err;
		if (register_io_dev_mmc(&s32g_mmc_io_conn))
			goto err;
		if (io_dev_open(s32g_mmc_io_conn,
				(uintptr_t)get_image_spec_from_id(FIP_IMAGE_ID),
				&s32g_mmc_dev_handle))
			goto err;
		if (io_dev_init(s32g_mmc_dev_handle, FIP_IMAGE_ID))
			goto err;
	}

	return;
err:
	ERROR("Error: %s failed\n", __func__);
	panic();
}
