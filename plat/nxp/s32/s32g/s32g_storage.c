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

static const io_dev_connector_t *s32g_fip_io_conn;
static uintptr_t s32g_fip_dev_handle;

static const io_dev_connector_t *s32g_memmap_io_conn;
static uintptr_t s32g_memmap_dev_handle;

static int s32g_check_mmc_dev(const uintptr_t spec);
static int s32g_check_fip_dev(const uintptr_t spec);
static int s32g_check_memmap_dev(const uintptr_t spec);

static io_block_spec_t fip_mmc_spec = {
	.offset = FIP_MMC_OFFSET,
};

static const io_block_spec_t fip_memmap_spec = {
	.offset = FIP_BASE,
	.length = ROUND_TO_MMC_BLOCK_SIZE(FIP_MAXIMUM_SIZE),
};

static const io_uuid_spec_t bl31_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
};

#ifdef SPD_opteed
static const io_uuid_spec_t bl32_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32,
};

static const io_uuid_spec_t bl32_extra1_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32_EXTRA1,
};
#endif

static const io_uuid_spec_t bl33_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
};

static io_block_spec_t qspi_fip_memmap_spec = {
	.offset = FIP_QSPI_OFFSET,
	.length = FIP_MAXIMUM_SIZE,
};

#ifdef FIP_MEM_OFFSET
static io_block_spec_t mem_fip_memmap_spec = {
	.offset = FIP_MEM_OFFSET,
	.length = FIP_MAXIMUM_SIZE,
};
#endif

static struct plat_io_policy s32g_policies[] = {
	[BL31_IMAGE_ID] = {
		&s32g_fip_dev_handle,
		(uintptr_t)&bl31_uuid_spec,
		s32g_check_fip_dev
	},
#ifdef SPD_opteed
	[BL32_IMAGE_ID] = {
		&s32g_fip_dev_handle,
		(uintptr_t)&bl32_uuid_spec,
		s32g_check_fip_dev
	},
	[BL32_EXTRA1_IMAGE_ID] = {
		&s32g_fip_dev_handle,
		(uintptr_t)&bl32_extra1_uuid_spec,
		s32g_check_fip_dev
	},
#endif
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

static void set_fip_img_source(struct plat_io_policy *policy)
{
	bl_mem_params_node_t *fip_params =
			get_bl_mem_params_node(FIP_IMAGE_ID);
	image_info_t *image_info;
	io_block_spec_t *io_mem_spec = &qspi_fip_memmap_spec;
	bool use_mem_offset = false;

	/* No need to check boot_source value integrity here.
	 * If the previous check had failed, the boot flow would
	 * not have reached this point.
	 */
	uint8_t boot_source = get_boot_source();

	if (fip_params == NULL) {
		ERROR("Invalid (NULL) fip parameters\n");
		return;
	}
	image_info = &fip_params->image_info;

	/* We know the real FIP image length only after FIP header
	 * is read and parsed in bl2_plat_handle_post_image_load.
	 * This code will be executed twice: the first time when the
	 * FIP header is read and the image_size will be FIP_HEADER_SIZE
	 * and second time when the entire FIP is read and image_size
	 * will be the one obtained in bl2_plat_handle_post_image_load.
	 */

#ifdef FIP_MEM_OFFSET
	/* Independent of the boot source, read FIP from Memory */
	io_mem_spec = &mem_fip_memmap_spec;
	use_mem_offset = true;
#endif

	if ((use_mem_offset == true) ||
		(boot_source == BOOT_SOURCE_QSPI)) {
		/* io_dev_init will check the header before image_info->image_size
		 * is computed.
		 */
		io_mem_spec->length = image_info->image_size ?
			image_info->image_size : io_mem_spec->length;
		*policy = (struct plat_io_policy) {
			.dev_handle = &s32g_memmap_dev_handle,
			.image_spec = (uintptr_t)io_mem_spec,
			.check = s32g_check_memmap_dev,
		};
	} else {
		fip_mmc_spec.length = image_info->image_size ?
			ROUND_TO_MMC_BLOCK_SIZE(image_info->image_size) :
			fip_mmc_spec.length;
		*policy = (struct plat_io_policy) {
			.dev_handle = &s32g_mmc_dev_handle,
			.image_spec = (uintptr_t)&fip_mmc_spec,
			.check = s32g_check_mmc_dev,
		};
	}
}

int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	const struct plat_io_policy *policy;
	int ret;

	assert(image_id < ARRAY_SIZE(s32g_policies));

	set_fip_img_source(&s32g_policies[FIP_IMAGE_ID]);

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
		if (s32g274a_mmc_register(boot_source))
			goto err;
		if (register_io_dev_mmc(&s32g_mmc_io_conn))
			goto err;
		if (io_dev_open(s32g_mmc_io_conn, (uintptr_t)&fip_mmc_spec,
				&s32g_mmc_dev_handle))
			goto err;
		if (io_dev_init(s32g_mmc_dev_handle, FIP_IMAGE_ID))
			goto err;
	}

	if (register_io_dev_fip(&s32g_fip_io_conn))
		goto err;
	if (io_dev_open(s32g_fip_io_conn, (uintptr_t)&bl31_uuid_spec,
			&s32g_fip_dev_handle))
		goto err;
	if (io_dev_init(s32g_fip_dev_handle, FIP_IMAGE_ID))
		goto err;

	return;
err:
	ERROR("Error: %s failed\n", __func__);
	panic();
}
