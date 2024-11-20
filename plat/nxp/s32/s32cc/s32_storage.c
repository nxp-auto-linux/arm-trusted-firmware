/*
 * Copyright 2019-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/bl_common.h>
#include <common/desc_image_load.h>
#include <common/fdt_wrappers.h>
#include <drivers/io/io_driver.h>
#include <drivers/mmc.h>
#include <drivers/io/io_memmap.h>
#include <drivers/io/io_fip.h>
#include <drivers/io/io_block.h>
#include <drivers/nxp/s32/mmc/s32_mmc.h>
#include <drivers/partition/partition.h>
#include <assert.h>
#include <tools_share/firmware_image_package.h>
#include <lib/mmio.h>
#include <libfdt.h>
#include <platform.h>
#include <s32cc_dt.h>

#include "s32cc_pinctrl.h"
#include "s32cc_storage.h"
#include "s32cc_bl_common.h"

#define QUADSPI_MCR			0x00000000u
#define QUADSPI_MCR_SWRSTHD_MASK	BIT(1)
#define QUADSPI_MCR_SWRSTSD_MASK	BIT(0)
#define QUADSPI_MCR_MDIS_MASK		BIT(14)

static const io_dev_connector_t *s32_mmc_io_conn;
static const io_dev_connector_t *s32_memmap_io_conn;
static const io_dev_connector_t *fip_dev_con;
static uintptr_t boot_dev_handle;
static uintptr_t fip_dev_handle;

static io_block_spec_t fip_memmap_spec;

static io_block_spec_t fip_mmc_spec;

static const io_block_dev_spec_t mmc_dev_spec = {
	.buffer	= {
		.offset = S32_MMC_BUFFER_BASE,
		.length = S32_MMC_BUFFER_SIZE
	},
	.ops = {
		.read = mmc_read_blocks,
	},
	.block_size = MMC_BLOCK_SIZE,
};

static io_block_spec_t mbr_spec = {
	.offset = 0,
	.length = PLAT_PARTITION_BLOCK_SIZE,
};

static const io_uuid_spec_t bl31_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
};

static const io_uuid_spec_t bl33_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
};

#ifdef SPD_opteed
static const io_uuid_spec_t bl32_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32,
};

static const io_uuid_spec_t bl32_extra1_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32_EXTRA1,
};
#endif

#if TRUSTED_BOARD_BOOT
static const io_uuid_spec_t soc_fw_key_cert_uuid_spec = {
	.uuid = UUID_SOC_FW_KEY_CERT,
};

static const io_uuid_spec_t soc_fw_content_cert_uuid_spec = {
	.uuid = UUID_SOC_FW_CONTENT_CERT,
};

static const io_uuid_spec_t tos_fw_key_cert_uuid_spec = {
	.uuid = UUID_TRUSTED_OS_FW_KEY_CERT,
};

static const io_uuid_spec_t tos_fw_content_cert_uuid_spec = {
	.uuid = UUID_TRUSTED_OS_FW_CONTENT_CERT,
};

static const io_uuid_spec_t nt_fw_key_cert_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FW_KEY_CERT,
};

static const io_uuid_spec_t nt_fw_content_cert_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FW_CONTENT_CERT,
};
#endif

static int check_dev(const uintptr_t spec)
{
	uintptr_t img_handle = 0;
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
	uintptr_t img_handle = 0;
	int result;

	result = io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);
	if (result)
		return result;

	result = io_open(fip_dev_handle, spec, &img_handle);
	if (result == 0)
		(void)io_close(img_handle);

	return result;
}

static struct plat_io_policy s32_policies[] = {
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
#ifdef SPD_opteed
	[BL32_IMAGE_ID] = {
		.dev_handle = &fip_dev_handle,
		.image_spec = (uintptr_t)&bl32_uuid_spec,
		.check = check_fip,
	},
	[BL32_EXTRA1_IMAGE_ID] = {
		.dev_handle = &fip_dev_handle,
		.image_spec = (uintptr_t)&bl32_extra1_uuid_spec,
		.check = check_fip,
	},
#endif

#if TRUSTED_BOARD_BOOT
	[SOC_FW_KEY_CERT_ID] = {
		.dev_handle = &fip_dev_handle,
		.image_spec = (uintptr_t)&soc_fw_key_cert_uuid_spec,
		.check = check_fip,
	},
	[SOC_FW_CONTENT_CERT_ID] = {
		.dev_handle = &fip_dev_handle,
		.image_spec = (uintptr_t)&soc_fw_content_cert_uuid_spec,
		.check = check_fip,
	},
	[TRUSTED_OS_FW_KEY_CERT_ID] = {
		.dev_handle = &fip_dev_handle,
		.image_spec = (uintptr_t)&tos_fw_key_cert_uuid_spec,
		.check = check_fip,
	},
	[TRUSTED_OS_FW_CONTENT_CERT_ID] = {
		.dev_handle = &fip_dev_handle,
		.image_spec = (uintptr_t)&tos_fw_content_cert_uuid_spec,
		.check = check_fip,
	},
	[NON_TRUSTED_FW_KEY_CERT_ID] = {
		.dev_handle = &fip_dev_handle,
		.image_spec = (uintptr_t)&nt_fw_key_cert_uuid_spec,
		.check = check_fip,
	},
	[NON_TRUSTED_FW_CONTENT_CERT_ID] = {
		.dev_handle = &fip_dev_handle,
		.image_spec = (uintptr_t)&nt_fw_content_cert_uuid_spec,
		.check = check_fip,
	},
#endif
	[GPT_IMAGE_ID] = {
		.dev_handle = &boot_dev_handle,
		.image_spec = (uintptr_t)&mbr_spec,
		.check = check_dev,
	},
};

static unsigned long get_fip_offset(void)
{
	if (fip_location_mmc)
		return fip_mmc_spec.offset;

	return fip_memmap_spec.offset;
}

static void invalidate_qspi_ahb(void)
{
	uint32_t reset_mask = QUADSPI_MCR_SWRSTHD_MASK | QUADSPI_MCR_SWRSTSD_MASK;

	mmio_clrbits_32(S32_QSPI_BASE + QUADSPI_MCR, QUADSPI_MCR_MDIS_MASK);
	mmio_setbits_32(S32_QSPI_BASE + QUADSPI_MCR, reset_mask);
	mmio_setbits_32(S32_QSPI_BASE + QUADSPI_MCR, QUADSPI_MCR_MDIS_MASK);
	mmio_clrbits_32(S32_QSPI_BASE + QUADSPI_MCR, reset_mask);
	mmio_clrbits_32(S32_QSPI_BASE + QUADSPI_MCR, QUADSPI_MCR_MDIS_MASK);
}

int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	const struct plat_io_policy *policy;
	int result __unused;

	assert(image_id < ARRAY_SIZE(s32_policies));

	policy = &s32_policies[image_id];

	result = policy->check(policy->image_spec);
	assert(result == 0);

	*dev_handle = *(policy->dev_handle);
	*image_spec = policy->image_spec;

	return 0;
}

static void plat_s32_mmc_setup(void)
{
	int result;
	partition_entry_t fip_part;

	result = s32_plat_config_sdhc_pinctrl();
	if (result)
		panic();

	result = s32_mmc_register();
	if (result)
		panic();

	result = register_io_dev_block(&s32_mmc_io_conn);
	assert(result == 0);

	result = io_dev_open(s32_mmc_io_conn, (uintptr_t)&mmc_dev_spec,
			     &boot_dev_handle);
	assert(result == 0);

	result = mmap_add_dynamic_region(S32_MMC_BUFFER_BASE,
					 S32_MMC_BUFFER_BASE,
					 MMU_ROUND_UP_TO_PAGE(S32_MMC_BUFFER_SIZE),
					 MT_MEMORY | MT_RW | MT_SECURE);
	if (result < 0) {
		ERROR("MMC mapping: error %d\n", result);
		panic();
	}

	result = load_partition_table(GPT_IMAGE_ID);
	if (result != 0) {
		ERROR("Could not load MBR partition table\n");
		panic();
	}

	fip_part = get_partition_entry_list()->list[FIP_PART];
	fip_mmc_spec.offset = fip_part.start;
	fip_mmc_spec.length = fip_part.length;
}

static void plat_s32_memmap_setup(void)
{
	int result;

	result = register_io_dev_memmap(&s32_memmap_io_conn);
	assert(result == 0);

	result = io_dev_open(s32_memmap_io_conn, (uintptr_t)NULL,
			     &boot_dev_handle);
	assert(result == 0);

	/* Mapping the rest of the images from the FIP */
	result = mmap_add_dynamic_region(fip_mem_offset, fip_mem_offset,
					 FIP_MAXIMUM_SIZE,
					 MT_RO | MT_MEMORY | MT_SECURE);
	if (result < 0) {
		ERROR("FIP mapping: error %d\n", result);
		panic();
	}

	fip_memmap_spec.offset = fip_mem_offset;
	fip_memmap_spec.length = FIP_MAXIMUM_SIZE;
}

static int get_qspi_fip_part(uintptr_t *base, size_t *size)
{
	int offs, subnode, parent_node, len = 0;
	const char *label_value;
	void *s32_fdt = NULL;

	if (dt_open_and_check() < 0)
		return -FDT_ERR_BADSTATE;

	if (fdt_get_address(&s32_fdt) == 0) {
		ERROR("No Device Tree found");
		return -FDT_ERR_BADSTATE;
	}

	offs = fdt_node_offset_by_compatible(s32_fdt, -1, "nxp,s32cc-qspi");
	if (offs < 0)
		return offs;

	if (fdt_get_status(offs) != DT_ENABLED)
		return -FDT_ERR_BADSTATE;

	offs = fdt_node_offset_by_compatible(s32_fdt, offs, "fixed-partitions");
	if (offs < 0)
		return offs;

	parent_node = offs;

	fdt_for_each_subnode(subnode, s32_fdt, parent_node) {
		label_value = fdt_getprop(s32_fdt, subnode, "label", &len);
		if (!label_value || len < 0)
			continue;

		if (strncmp(label_value, "FIP", (size_t)len) == 0)
			break;
	}

	if (subnode < 0)
		return subnode;

	return fdt_get_reg_props_by_index(s32_fdt, subnode, 0, base, size);
}

static void plat_s32_qspi_setup(void)
{
	int result;
	uintptr_t fip_base = 0;
	size_t fip_size = 0;

	result = get_qspi_fip_part(&fip_base, &fip_size);
	if (result || !fip_base || !fip_size)
		panic();

	result = register_io_dev_memmap(&s32_memmap_io_conn);
	assert(result == 0);

	result = io_dev_open(s32_memmap_io_conn, (uintptr_t)NULL,
			     &boot_dev_handle);
	assert(result == 0);

	invalidate_qspi_ahb();

	result = mmap_add_dynamic_region(S32_FLASH_BASE + fip_base,
					 S32_FLASH_BASE + fip_base,
					 MMU_ROUND_UP_TO_PAGE(fip_size),
					 MT_RO | MT_MEMORY | MT_SECURE);
	if (result < 0) {
		ERROR("QSPI mapping: error %d\n", result);
		panic();
	}

	fip_memmap_spec.offset = S32_FLASH_BASE + fip_base;
	fip_memmap_spec.length = fip_size;
}

void s32_io_setup(void)
{
	int result __maybe_unused;

	if (fip_location_mem) {
		plat_s32_memmap_setup();
		s32_policies[FIP_IMAGE_ID].image_spec = (uintptr_t)&fip_memmap_spec;
	}

	if (fip_location_qspi) {
		plat_s32_qspi_setup();
		s32_policies[FIP_IMAGE_ID].image_spec = (uintptr_t)&fip_memmap_spec;
	}

	if (fip_location_mmc) {
		plat_s32_mmc_setup();
		s32_policies[FIP_IMAGE_ID].image_spec = (uintptr_t)&fip_mmc_spec;
	}

	result = register_io_dev_fip(&fip_dev_con);
	assert(result == 0);

	result = io_dev_open(fip_dev_con, (uintptr_t)NULL, &fip_dev_handle);
	assert(result == 0);

	INFO("BL2: FIP offset = 0x%lx\n", get_fip_offset());
}
