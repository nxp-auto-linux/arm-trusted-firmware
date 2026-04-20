/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>

#include <common/debug.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

#include <drivers/nxp/s32n/s32n_xrdc_aida.h>
#include <fconf/s32n_fconf_smmu_getter.h>
#include <fconf/s32n_fconf_xrdc_aida_getter.h>

#define XRDC_AIDA_AD_SIZE	(0x1000)

#define XRDC_AIDA_AD_DAUXCR_OFFSET	(0x484)
#define XRDC_AIDA_AD_DAUXCRn_OFFSET(n)	(XRDC_AIDA_AD_DAUXCR_OFFSET + (n) * 4)
#define XRDC_AIDA_AD_STREAMID_MASK	GENMASK_32(15, 0)
#define XRDC_AIDA_AD_DAUX_MS_MASK	GENMASK_32(31, 16)

#define COSS_XRDC_AIDA_AD7_BASE		(0x4A3C7000U) /* UFSHC */
#define FSS_XRDC_AIDA_AD6_BASE		(0x5B3D2000U) /* USDHC */

#define COSS_UFSHC_BASE			(0x4AC80000U)
#define COSS_UFSHCI0_BASE		(0x4ACB0000U)
#define COSS_UFSHCI1_BASE		(0x4ACD0000U)
#define COSS_UFSHCI2_BASE		(0x4ACF0000U)
#define COSS_UFSHCI3_BASE		(0x4AD10000U)
#define FSS_USDHC_BASE			(0x5B490000U)

/**
 * struct xrdc_aida - XRDC AIDA instance
 * @aida_base: Base address of the XRDC AIDA instance
 * @mapped: Flag indicating whether the AIDA instance is memory mapped
 *
 * This structure represents an XRDC AIDA instance, containing its base address
 * and memory mapping status.
 */
struct xrdc_aida {
	uintptr_t aida_base;
	bool mapped;
};

enum xrdc_aida_instance {
	FSS_XRDC_AIDA_AD6,
	COSS_XRDC_AIDA_AD7,
};

/**
 * struct xrdc_aida_to_dev_mapping - XRDC AIDA to device mapping
 * @aida: Pointer to XRDC AIDA instance
 * @dev_base: Base address of the associated device/master
 * @daux_entry_id: DAUX entry identifier for the device
 *
 * This structure defines the mapping between XRDC AIDA instances and their
 * associated devices, used for configuring stream ID assignments.
 */
struct xrdc_aida_to_dev_mapping {
	struct xrdc_aida *aida;
	uintptr_t dev_base;
	uint32_t daux_entry_id;
};

static struct xrdc_aida s32n_xrdc_aida_instances[] = {
	[FSS_XRDC_AIDA_AD6] = { .aida_base = FSS_XRDC_AIDA_AD6_BASE, .mapped = false },
	[COSS_XRDC_AIDA_AD7] = { .aida_base = COSS_XRDC_AIDA_AD7_BASE, .mapped = false },
};

static const struct xrdc_aida_to_dev_mapping s32n_xrdc_aida_to_dev_map[] = {
	{ /* USDHC */
		.aida = &s32n_xrdc_aida_instances[FSS_XRDC_AIDA_AD6],
		.dev_base = FSS_USDHC_BASE,
		.daux_entry_id = 0,
	},
	{ /* UFSHC */
		.aida = &s32n_xrdc_aida_instances[COSS_XRDC_AIDA_AD7],
		.dev_base = COSS_UFSHC_BASE,
		.daux_entry_id = 30,
	},
	{ /* UFSHCI0 */
		.aida = &s32n_xrdc_aida_instances[COSS_XRDC_AIDA_AD7],
		.dev_base = COSS_UFSHCI0_BASE,
		.daux_entry_id = 0, /* HID 0 */
	},
	{ /* UFSHCI1 */
		.aida = &s32n_xrdc_aida_instances[COSS_XRDC_AIDA_AD7],
		.dev_base = COSS_UFSHCI1_BASE,
		.daux_entry_id = 1, /* HID 1 */
	},
	{ /* UFSHCI2 */
		.aida = &s32n_xrdc_aida_instances[COSS_XRDC_AIDA_AD7],
		.dev_base = COSS_UFSHCI2_BASE,
		.daux_entry_id = 2, /* HID 2 */
	},
	{ /* UFSHCI3 */
		.aida = &s32n_xrdc_aida_instances[COSS_XRDC_AIDA_AD7],
		.dev_base = COSS_UFSHCI3_BASE,
		.daux_entry_id = 3, /* HID 3 */
	},
};

/**
 * s32n_xrdc_aida_find_mapping - Find XRDC AIDA mapping for a master device
 * @master_base: Base address of the master device to search for
 *
 * This function searches through the XRDC AIDA to device mapping table to find
 * the mapping entry that corresponds to the specified master device base address.
 *
 * Return: Pointer to the matching xrdc_aida_to_dev_mapping structure if found,
 *         NULL if no matching mapping is found.
 */
static inline const struct xrdc_aida_to_dev_mapping *
s32n_xrdc_aida_find_mapping(uintptr_t master_base)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(s32n_xrdc_aida_to_dev_map); i++) {
		if (master_base == s32n_xrdc_aida_to_dev_map[i].dev_base)
			return &s32n_xrdc_aida_to_dev_map[i];
	}

	return NULL;
}

static int s32n_xrdc_aida_write_daux(uintptr_t aida_base,
				     uint32_t daux_entry_id,
				     uint32_t stream_id, uint32_t daux_ms)
{
	if (XRDC_AIDA_AD_DAUXCRn_OFFSET(daux_entry_id) >= XRDC_AIDA_AD_SIZE) {
		ERROR("XRDC-AIDA[0x%lx]: DAUX id %u out of range\n",
		      aida_base, daux_entry_id);
		return -EINVAL;
	}

	if (stream_id & ~XRDC_AIDA_AD_STREAMID_MASK) {
		ERROR("XRDC-AIDA[0x%lx]: StreamID 0x%x exceeds 16 bits\n",
		      aida_base, stream_id);
		return -EINVAL;
	}

	stream_id &= XRDC_AIDA_AD_STREAMID_MASK;
	mmio_clrsetbits_32(aida_base + XRDC_AIDA_AD_DAUXCRn_OFFSET(daux_entry_id),
			   XRDC_AIDA_AD_STREAMID_MASK, stream_id);

	if (daux_ms & ~XRDC_AIDA_AD_DAUX_MS_MASK) {
		ERROR("XRDC-AIDA[0x%lx]: DAUX_MS 0x%x exceeds 16 bits\n",
		      aida_base, daux_ms);
		return -EINVAL;
	}

	/*
	 * TODO: decide whether to clean DAUX_MS bits (they might have been
	 * configured by the Partition Manager). For now, keep them as is.
	 */
	daux_ms &= XRDC_AIDA_AD_DAUX_MS_MASK;
	mmio_clrsetbits_32(aida_base + XRDC_AIDA_AD_DAUXCRn_OFFSET(daux_entry_id),
			   0, daux_ms);

	return 0;
}

static void s32n_xrdc_aida_get_config(struct xrdc_aida_config *cfg_tbl)
{
	cfg_tbl->entries = FCONF_GET_PROPERTY(hw_config, xrdc_aida_config,
					      entries);
	cfg_tbl->entries_n_elem = FCONF_GET_PROPERTY(hw_config, xrdc_aida_config,
						     entries_n_elem);
}

static inline bool s32n_xrdc_aida_is_smmu_enabled(void)
{
	return !!(FCONF_GET_PROPERTY(hw_config, smmu_config, enabled));
}

static int s32n_xrdc_aida_mmap_instance(struct xrdc_aida *inst)
{
	uintptr_t aida_base = inst->aida_base;
	int ret;

	if (inst->mapped) {
		VERBOSE("XRDC-AIDA: base 0x%lx already mapped\n", aida_base);
		return 0;
	}

	ret = mmap_add_dynamic_region(aida_base, aida_base, XRDC_AIDA_AD_SIZE,
				      MT_DEVICE | MT_RW | MT_SECURE);
	if (ret) {
		ERROR("XRDC-AIDA: failed to map base=0x%lx (ret=%d)\n",
		      aida_base, ret);
		return ret;
	}

	inst->mapped = true;
	VERBOSE("XRDC-AIDA: mapped AIDA@0x%lx size=0x%x\n",
		aida_base, XRDC_AIDA_AD_SIZE);

	return 0;
}

int s32n_xrdc_aida_init(void)
{
	const struct xrdc_aida_to_dev_mapping *mapping;
	struct xrdc_aida_config_entry *fconf_entry;
	struct xrdc_aida_config cfg_tbl;
	struct xrdc_aida *aida;
	uint32_t i;
	int ret;

	if (!s32n_xrdc_aida_is_smmu_enabled()) {
		INFO("XRDC-AIDA: SMMU not enabled, skipping initialization\n");
		return 0;
	}

	s32n_xrdc_aida_get_config(&cfg_tbl);

	if (cfg_tbl.entries_n_elem == 0) {
		INFO("XRDC-AIDA: No entries to be configured\n");
		return 0;
	}

	for (i = 0; i < cfg_tbl.entries_n_elem; i++) {
		fconf_entry = &cfg_tbl.entries[i];
		mapping = s32n_xrdc_aida_find_mapping(fconf_entry->base_addr);
		if (!mapping) {
			WARN("XRDC-AIDA: no instance for dev_base=0x%lx (idx %u) SID=0x%x\n DAUX_MS=0x%x",
			     fconf_entry->base_addr, i, fconf_entry->stream_id,
			     fconf_entry->daux_ms);
			return -ENOENT;
		}

		aida = mapping->aida;
		ret = s32n_xrdc_aida_mmap_instance(aida);
		if (ret)
			return ret;

		ret = s32n_xrdc_aida_write_daux(aida->aida_base,
						mapping->daux_entry_id,
						fconf_entry->stream_id,
						fconf_entry->daux_ms);
		if (ret)
			return ret;

		VERBOSE("XRDC-AIDA: dev_base=0x%lx -> AIDA@0x%lx SID=0x%x DAUX_MS=0x%x\n",
			fconf_entry->base_addr, aida->aida_base,
			fconf_entry->stream_id, fconf_entry->daux_ms);
	}

	INFO("XRDC-AIDA: programmed %u entries\n", cfg_tbl.entries_n_elem);
	return 0;
}
