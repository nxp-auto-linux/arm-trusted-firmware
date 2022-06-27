/*
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>

#include <common/bl_common.h>
#include <common/debug.h>
#include <common/desc_image_load.h>
#include <common/fdt_fixup.h>
#include <common/fdt_wrappers.h>
#include <ddr/ddr_density.h>
#include <ddr/ddr_utils.h>
#include <lib/libc/errno.h>
#include <lib/libfdt/libfdt.h>
#include <lib/mmio.h>
#include <lib/optee_utils.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <platform.h>
#include <s32_bl_common.h>
#include <tools_share/firmware_image_package.h>

#if (ERRATA_S32_050543 == 1)
#include <dt-bindings/ddr-errata/s32-ddr-errata.h>
#endif
#include "s32_dt.h"
#include "s32_clocks.h"
#include "s32_mc_me.h"
#include "s32_mc_rgm.h"
#include "s32_sramc.h"
#include "s32_storage.h"

#define S32_FDT_UPDATES_SPACE		100U

#define AARCH64_UNCOND_BRANCH_MASK	(0x7c000000)
#define AARCH64_UNCOND_BRANCH_OP	(BIT(26) | BIT(28))
#define BL33_DTB_MAGIC				(0xedfe0dd0)

#define PER_GROUP3_BASE		(0x40300000UL)
#define FCCU_BASE_ADDR		(PER_GROUP3_BASE + 0x0000C000)
#define FCCU_NCF_S1			(FCCU_BASE_ADDR + 0x84)
#define FCCU_NCFK			(FCCU_BASE_ADDR + 0x90)
#define FCCU_NCFK_KEY		(0xAB3498FE)

#define MEMORY_STRING		"memory"

void add_fip_img_to_mem_params_descs(bl_mem_params_node_t *params,
					    size_t *index)
{
	params[(*index)++] = (bl_mem_params_node_t) {
		.image_id = FIP_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      NON_SECURE | EXECUTABLE),

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, IMAGE_ATTRIB_PLAT_SETUP),
		.image_info.image_max_size = FIP_MAXIMUM_SIZE,
		.image_info.image_size = FIP_HEADER_SIZE,
		.image_info.image_base = FIP_BASE,
		.next_handoff_image_id = BL31_IMAGE_ID,
	};
}

void add_bl31_img_to_mem_params_descs(bl_mem_params_node_t *params,
					     size_t *index)
{
	params[(*index)++] = (bl_mem_params_node_t) {
		.image_id = BL31_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      SECURE | EXECUTABLE | EP_FIRST_EXE),
		.ep_info.spsr = SPSR_64(MODE_EL3, MODE_SP_ELX,
					DISABLE_ALL_EXCEPTIONS),
		.ep_info.pc = BL31_BASE,

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, 0),
		.image_info.image_max_size = BL31_LIMIT - BL31_BASE,
		.image_info.image_base = BL31_BASE,
#ifdef SPD_opteed
		.next_handoff_image_id = BL32_IMAGE_ID,
#else
		.next_handoff_image_id = BL33_IMAGE_ID,
#endif
	};
}

#ifdef SPD_opteed
void add_bl32_img_to_mem_params_descs(bl_mem_params_node_t *params,
					     size_t *index)
{
	params[(*index)++] = (bl_mem_params_node_t) {
		.image_id = BL32_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      SECURE | EXECUTABLE),
		.ep_info.pc = S32_BL32_BASE,

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, 0),
		.image_info.image_max_size = S32_BL32_SIZE,
		.image_info.image_base = S32_BL32_BASE,
		.next_handoff_image_id = BL33_IMAGE_ID,
	};
}

void add_bl32_extra1_img_to_mem_params_descs(
	bl_mem_params_node_t *params,
	size_t *index)
{
	params[(*index)++] = (bl_mem_params_node_t) {

		.image_id = BL32_EXTRA1_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      SECURE | NON_EXECUTABLE),

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, IMAGE_ATTRIB_SKIP_LOADING),
		.image_info.image_base = S32_BL32_BASE,
		.image_info.image_max_size = S32_BL32_SIZE,

		.next_handoff_image_id = INVALID_IMAGE_ID,
	};
}

#else
void add_bl32_img_to_mem_params_descs(bl_mem_params_node_t *params,
					     size_t *index)
{

}

void add_bl32_extra1_img_to_mem_params_descs(
	bl_mem_params_node_t *params,
	size_t *index)
{

}
#endif /* SPD_opteed */

void add_bl33_img_to_mem_params_descs(bl_mem_params_node_t *params,
					     size_t *index)
{
	bl_mem_params_node_t node = {
		.image_id = BL33_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      NON_SECURE | EXECUTABLE),

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, 0),
		.image_info.image_max_size = S32_BL33_IMAGE_SIZE,
		.image_info.image_base = S32_BL33_IMAGE_BASE,
		.next_handoff_image_id = INVALID_IMAGE_ID,
	};

	params[(*index)++] = node;
}

void add_invalid_img_to_mem_params_descs(bl_mem_params_node_t *params,
						size_t *index)
{
	bl_mem_params_node_t node = {
		.image_id = INVALID_IMAGE_ID,
		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, IMAGE_ATTRIB_SKIP_LOADING),
	};

	params[(*index)++] = node;
}

IMPORT_SYM(uintptr_t, __RW_START__, BL2_RW_START);

static uintptr_t get_fip_hdr_page(void)
{
	return get_fip_hdr_base() & ~PAGE_MASK;
}

static mmap_region_t s32_mmap[] = {
#if !defined(PLAT_s32r)
	MAP_REGION_FLAT(S32G_SSRAM_BASE, S32G_SSRAM_LIMIT - S32G_SSRAM_BASE,
			 MT_MEMORY | MT_RW | MT_SECURE),
#endif
	MAP_REGION_FLAT(S32_UART_BASE, S32_UART_SIZE,
			MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(S32_MC_ME_BASE_ADDR, S32_MC_ME_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(MC_CGM0_BASE_ADDR,
			MMU_ROUND_UP_TO_PAGE(S32_DFS_ADDR(S32_DFS_NR)),
			MT_DEVICE | MT_RW),
	/* This will cover both the MC_RGM and the GPR accesses, while reducing
	 * the number of used up MMU regions.
	 */
	MAP_REGION_FLAT(S32_MC_RGM_BASE_ADDR, S32_MC_RGM_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(SRAMC0_BASE_ADDR, SRAMC_SIZE,
			MT_DEVICE | MT_RW),
#if !defined(PLAT_s32r)
	MAP_REGION_FLAT(SSRAMC_BASE_ADDR, SRAMC_SIZE,
			MT_DEVICE | MT_RW),
#endif
	MAP_REGION2(S32_BL32_BASE, S32_BL32_BASE,
			MMU_ROUND_UP_TO_PAGE(S32_BL32_SIZE),
			MT_MEMORY | MT_RW, PAGE_SIZE),
	MAP_REGION2(BL33_DTB, BL33_DTB,
			MMU_ROUND_UP_TO_PAGE(S32_BL33_IMAGE_SIZE),
			MT_MEMORY | MT_RW, PAGE_SIZE),
	MAP_REGION_FLAT(S32_PMEM_START, S32_PMEM_LEN,
			MT_MEMORY | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32_SCMI_SHARED_MEM, S32_SCMI_SHARED_MEM_SIZE,
			MT_NON_CACHEABLE | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32_QSPI_BASE, S32_QSPI_SIZE, MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(FIP_BASE, FIP_MAXIMUM_SIZE, MT_RW | MT_SECURE),
#if (ERRATA_S32_050543 == 1)
	MAP_REGION_FLAT(DDR_ERRATA_REGION_BASE, DDR_ERRATA_REGION_SIZE,
			MT_NON_CACHEABLE | MT_RW),
#endif
	MAP_REGION_FLAT(S32_FLASH_BASE, FIP_MAXIMUM_SIZE, MT_RW | MT_SECURE),
	{0},
};

static int disable_qspi_mmu_entry(void)
{
	int offset;
	void *fdt = NULL;
	size_t i;

	if (dt_open_and_check() < 0) {
		ERROR("Failed to check FDT integrity\n");
		return -EFAULT;
	}

	if (fdt_get_address(&fdt) == 0) {
		ERROR("Failed to get FDT address\n");
		return -EFAULT;
	}

	offset = fdt_node_offset_by_compatible(fdt, -1, "nxp,s32cc-qspi");
	if (offset > 0) {
		if (fdt_get_status(offset) == DT_ENABLED)
			return 0;
	}

	for (i = 0; i < ARRAY_SIZE(s32_mmap); i++) {
		if (s32_mmap[i].base_pa == S32_FLASH_BASE) {
			s32_mmap[i].size = 0;
			break;
		}
	}

	return 0;
}

#ifdef HSE_SECBOOT
static size_t get_fip_size(void)
{
	static const uuid_t uuid_null = { {0} };
	uintptr_t fip_hdr_start, fip_hdr_end;
	fip_toc_header_t *toc_header;
	fip_toc_entry_t *toc_entry;

	fip_hdr_start = get_fip_hdr_base();
	fip_hdr_end = fip_hdr_start + fip_hdr_size;

	toc_header = (fip_toc_header_t *)fip_hdr_start;
	toc_entry = (fip_toc_entry_t *)(toc_header + 1);

	while ((uintptr_t)toc_entry < fip_hdr_end) {
		if (!compare_uuids(&toc_entry->uuid, &uuid_null))
			break;

		toc_entry++;
	}

	return (size_t)toc_entry->offset_address;
}
#endif

int s32_el3_mmu_fixup(void)
{
	const unsigned long code_start = BL_CODE_BASE;
	const unsigned long code_size = BL_CODE_END - BL_CODE_BASE;
	const unsigned long rw_start = BL2_RW_START;
	const unsigned long rw_size = BL_END - BL2_RW_START;
	mmap_region_t regions[] = {
		{
			.base_pa = code_start,
			.base_va = code_start,
			.size = code_size,
			.attr = MT_CODE | MT_SECURE,
		},
		{
			.base_pa = rw_start,
			.base_va = rw_start,
			.size = rw_size,
			.attr = MT_RW | MT_MEMORY | MT_SECURE,
		},
		/* FIP Header & DTB */
		{
			.base_pa = get_fip_hdr_page(),
			.base_va = get_fip_hdr_page(),
#ifdef HSE_SECBOOT
			.size = MMU_ROUND_UP_TO_PAGE(get_fip_size()),
#else
			.size = BL2_BASE - get_fip_hdr_page(),
#endif
			.attr = MT_RO | MT_MEMORY | MT_SECURE,
		},
	};
	int i, ret;

	ret = disable_qspi_mmu_entry();
	if (ret)
		return ret;

	/* Check the BL31/BL32/BL33 memory ranges for overlapping */
	_Static_assert(S32_BL32_BASE + S32_BL32_SIZE <= BL31_BASE,
				"BL32 and BL31 memory ranges overlap!");
	_Static_assert(BL31_BASE + BL31_SIZE <= BL33_BASE,
				"BL31 and BL33 memory ranges overlap!");

	/* The calls to mmap_add_region() consume mmap regions,
	 * so they must be counted in the static asserts
	 */
	_Static_assert(ARRAY_SIZE(s32_mmap) + ARRAY_SIZE(regions) - 1 <=
		MAX_MMAP_REGIONS,
		"Fewer MAX_MMAP_REGIONS than in s32_mmap will likely result in a MMU exception at runtime");

	/* MMU initialization; while technically not necessary, improves
	 * bl2_load_images execution time.
	 */
	for (i = 0; i < ARRAY_SIZE(regions); i++)
		mmap_add_region(regions[i].base_pa, regions[i].base_va,
				regions[i].size, regions[i].attr);

	mmap_add(s32_mmap);

	init_xlat_tables();
	enable_mmu_el3(0);

	return 0;
}

struct bl_load_info *plat_get_bl_image_load_info(void)
{
	return get_bl_load_info_from_mem_params_desc();
}

void bl2_platform_setup(void)
{

}

struct bl_params *plat_get_next_bl_params(void)
{
	return get_next_bl_params_from_mem_params_desc();
}

void plat_flush_next_bl_params(void)
{
	flush_bl_params_desc();
}

static bool is_branch_op(uint32_t op)
{
	return (op & AARCH64_UNCOND_BRANCH_MASK) == AARCH64_UNCOND_BRANCH_OP;
}

#if S32CC_EMU == 0
static int ft_fixup_exclude_ecc(void *blob)
{
	int ret, nodeoff = -1;
	bool found_first_node = false;
	unsigned long start = 0, size = 0;
	const char *node_name;

	/* Get offset of memory node */
	while ((nodeoff = fdt_node_offset_by_prop_value(blob, nodeoff,
			"device_type", MEMORY_STRING,
			sizeof(MEMORY_STRING))) >= 0) {
		found_first_node = true;

		node_name = fdt_get_name(blob, nodeoff, NULL);

		/* Get value of "reg" property */
		ret = fdt_get_reg_props_by_index(blob, nodeoff, 0, &start, &size);
		if (ret) {
			ERROR("Couldn't get 'reg' property values of %s node\n",
				node_name);
			return ret;
		}

		s32gen1_exclude_ecc(&start, &size);

		/* Delete old "reg" property */
		ret = fdt_delprop(blob, nodeoff, "reg");
		if (ret) {
			ERROR("Failed to remove 'reg' property of '%s' node\n",
				node_name);
			return ret;
		}

		/* Write newly-computed "reg" property values back to DT */
		ret = fdt_setprop_u64(blob, nodeoff, "reg", start);
		if (ret < 0) {
			ERROR("Cannot write 'reg' property of '%s' node\n",
				node_name);
			return ret;
		}

		ret = fdt_appendprop_u64(blob, nodeoff, "reg", size);
		if (ret < 0) {
			ERROR("Cannot write 'reg' property of '%s' node\n",
				node_name);
			return ret;
		}
	}

	if (nodeoff < 0 && !found_first_node) {
		ERROR("No memory node found\n");
		return nodeoff;
	}

	return 0;
}
#endif

static int ft_fixup_resmem_node(void *blob)
{
	int ret;

	ret = fdt_add_reserved_memory(blob, "atf", BL31_BASE, BL31_SIZE);
	if (ret) {
		ERROR("Failed to add 'atf' /reserved-memory node");
		return ret;
	}

	return 0;
}

static int ft_fixups(void *blob)
{
	size_t size = fdt_totalsize(blob);
	int ret;

	size += S32_FDT_UPDATES_SPACE;
	fdt_set_totalsize(blob, size);

#if S32CC_EMU == 0
	ret = ft_fixup_exclude_ecc(blob);
	if (ret)
		goto out;
#endif /* S32CC_EMU */

	ret = ft_fixup_resmem_node(blob);
	if (ret)
		goto out;

out:
	flush_dcache_range((uintptr_t)blob, size);
	return ret;
}

/* Computes the size of the images inside FIP and updates image io_block spec.
 * Only FIP header was read from storage to SRAM so we can walk through all
 * fip_toc_entry_t entries until the last one.
 * For each images it is updated the real size and offset as read from the
 * FIP header.
 */
static int set_fip_images_size(bl_mem_params_node_t *fip_params)
{
	static const uuid_t uuid_null = { {0} };
	image_info_t *image_info = &fip_params->image_info;
	char *buf = (char *)image_info->image_base;
	char *buf_end = buf + image_info->image_size;
	fip_toc_header_t *toc_header = (fip_toc_header_t *)buf;
	fip_toc_entry_t *toc_entry = (fip_toc_entry_t *)(toc_header + 1);

	while ((char *)toc_entry < buf_end) {
		if (compare_uuids(&toc_entry->uuid, &uuid_null) == 0)
			break;

		set_image_spec(&toc_entry->uuid, toc_entry->size,
			       toc_entry->offset_address);
		toc_entry++;
	}

	return 0;
}

int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	uint32_t magic;
	int ret;

	bl_mem_params_node_t *bl_mem_params = NULL;
	bl_mem_params_node_t *pager_mem_params = NULL;
	bl_mem_params_node_t *paged_mem_params = NULL;

	if (image_id == FIP_IMAGE_ID) {
		bl_mem_params = get_bl_mem_params_node(image_id);
		assert(bl_mem_params && "FIP params cannot be NULL");

		set_fip_images_size(bl_mem_params);
	}

	if (image_id == BL33_IMAGE_ID) {
		magic = mmio_read_32(BL33_ENTRYPOINT);
		if (!is_branch_op(magic)) {
			printf(
			    "Warning: Instruction at BL33_ENTRYPOINT (0x%x) is 0x%x, which is not a B or BL!\n",
			    BL33_ENTRYPOINT, magic);
		}

		if (get_bl2_dtb_size() > BL33_MAX_DTB_SIZE) {
			ERROR("The DTB exceeds max BL31 DTB size: 0x%x\n",
			      BL33_MAX_DTB_SIZE);
			return -EIO;
		}

		memcpy((void *)BL33_DTB, (void *)get_bl2_dtb_base(),
		       get_bl2_dtb_size());

		ret = ft_fixups((void *)BL33_DTB);
		if (ret)
			return ret;
	}

	if (image_id == BL32_IMAGE_ID) {
		bl_mem_params = get_bl_mem_params_node(image_id);
		assert(bl_mem_params && "bl_mem_params cannot be NULL");

		pager_mem_params = get_bl_mem_params_node(BL32_EXTRA1_IMAGE_ID);
		assert(pager_mem_params && "pager_mem_params cannot be NULL");

		ret = parse_optee_header(&bl_mem_params->ep_info,
					 &pager_mem_params->image_info,
					 &paged_mem_params->image_info);
		if (ret != 0) {
			WARN("OPTEE header parse error.\n");
			return ret;
		}
	}

	return 0;
}

/**
 * Clear non-critical faults generated by SWT (software watchdog timer)
 * All SWT faults are placed in NCF_S1 (33-38)
 */
void clear_swt_faults(void)
{
	unsigned int val = mmio_read_32(FCCU_NCF_S1);

	if (val) {
		mmio_write_32(FCCU_NCFK, FCCU_NCFK_KEY);
		mmio_write_32(FCCU_NCF_S1, val);
	}
}

void clear_reset_cause(void)
{
	uint32_t mc_rgm_des = mmio_read_32(MC_RGM_DES);

	mmio_write_32(MC_RGM_DES, mc_rgm_des);
}

const char *get_reset_cause_str(enum reset_cause reset_cause)
{
	static const char * const names[] = {
		[CAUSE_POR] = "Power-On Reset",
		[CAUSE_DESTRUCTIVE_RESET_DURING_RUN] = "Destructive Reset (RUN)",
		[CAUSE_DESTRUCTIVE_RESET_DURING_STANDBY] = "Destructive Reset (STBY)",
		[CAUSE_FUNCTIONAL_RESET_DURING_RUN] = "Functional Reset (RUN)",
		[CAUSE_FUNCTIONAL_RESET_DURING_STANDBY] = "Functional Reset (STBY)",
		[CAUSE_WAKEUP_DURING_STANDBY] = "Wakeup (STBY)",
		[CAUSE_ERROR] = "Error",
	};

	if (reset_cause >= ARRAY_SIZE(names))
		return "Unknown cause";

	return names[reset_cause];
}

