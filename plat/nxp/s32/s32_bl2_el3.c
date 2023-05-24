/*
 * Copyright 2021-2023 NXP
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
#include "ddr_utils.h"
#include <lib/libc/errno.h>
#include <lib/libfdt/libfdt.h>
#include <lib/mmio.h>
#include <lib/optee_utils.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <platform.h>
#include <s32_bl_common.h>
#include <tools_share/firmware_image_package.h>
#include <s32_bl2_el3.h>
#include <lib/bakery_lock.h>

#include <dt-bindings/nvmem/s32cc-scmi-nvmem.h>

/**
 * SCMI ATF to SCP communication requires locking API calls,
 * e.g. bakery_lock_get/release.
 *
 * However, during BL2, there is no concurrent access, thus, the lock is not
 * really needed. This provides the locking API with no real implementation
 * (dummy).
 */
void bakery_lock_get(bakery_lock_t *bakery)
{
}

void bakery_lock_release(bakery_lock_t *bakery)
{
}

/* No irq during bl31sram */
void plat_ic_set_interrupt_pending(unsigned int id)
{
}

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
#define AARCH64_UNCOND_BRANCH_OP			(BIT(26) | BIT(28))
#define AARCH64_UNCOND_BRANCH_OP_BOOT0_HOOK	(BIT(27) | BIT(28) | BIT(30))
#define BL33_DTB_MAGIC				(0xedfe0dd0)

#define PER_GROUP3_BASE		(0x40300000UL)
#define FCCU_BASE_ADDR		(PER_GROUP3_BASE + 0x0000C000)
#define FCCU_SIZE		(0x94)
#define FCCU_NCF_S1			(FCCU_BASE_ADDR + 0x84)
#define FCCU_NCFK			(FCCU_BASE_ADDR + 0x90)
#define FCCU_NCFK_KEY		(0xAB3498FE)

#define MEMORY_STRING		"memory"

#define DDRSS_BASE_ADDR		(0x40380000)
#define DDRSS_SIZE		(0x80000)

static const char *gpio_scmi_node_path = "/firmware/scmi/protocol@81";
static const char *nvmem_scmi_node_path = "/firmware/scmi/protocol@82";

int add_bl31_img_to_mem_params_descs(bl_mem_params_node_t *params,
				     size_t *index, size_t size)
{
	if (*index >= size)
		return -EINVAL;

	params[(*index)++] = (bl_mem_params_node_t) {
		.image_id = BL31_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      SECURE | EXECUTABLE | EP_FIRST_EXE),
		.ep_info.spsr = SPSR_64(MODE_EL3, MODE_SP_ELX,
					DISABLE_ALL_EXCEPTIONS),
		.ep_info.pc = BL31_BASE,

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, IMAGE_ATTRIB_PLAT_SETUP),
		.image_info.image_max_size = BL31_LIMIT - BL31_BASE,
		.image_info.image_base = BL31_BASE,
#ifdef SPD_opteed
		.next_handoff_image_id = BL32_IMAGE_ID,
#else
		.next_handoff_image_id = BL33_IMAGE_ID,
#endif
	};

	return 0;
}

#ifdef SPD_opteed
int add_bl32_img_to_mem_params_descs(bl_mem_params_node_t *params,
				     size_t *index, size_t size)
{
	if (*index >= size)
		return -EINVAL;

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

	return 0;
}

int add_bl32_extra1_img_to_mem_params_descs(bl_mem_params_node_t *params,
					    size_t *index, size_t size)
{
	if (*index >= size)
		return -EINVAL;

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

	return 0;
}

#else
int add_bl32_img_to_mem_params_descs(bl_mem_params_node_t *params,
				     size_t *index, size_t size)
{
	return 0;
}

int add_bl32_extra1_img_to_mem_params_descs(bl_mem_params_node_t *params,
					    size_t *index, size_t size)
{
	return 0;
}
#endif /* SPD_opteed */

int add_bl33_img_to_mem_params_descs(bl_mem_params_node_t *params,
				     size_t *index, size_t size)
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

	if (*index >= size)
		return -EINVAL;

	params[(*index)++] = node;

	return 0;
}

int add_invalid_img_to_mem_params_descs(bl_mem_params_node_t *params,
					size_t *index, size_t size)
{
	bl_mem_params_node_t node = {
		.image_id = INVALID_IMAGE_ID,
		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, IMAGE_ATTRIB_SKIP_LOADING),
	};

	if (*index >= size)
		return -EINVAL;

	params[(*index)++] = node;

	return 0;
}

IMPORT_SYM(uintptr_t, __RW_START__, BL2_RW_START);

static uintptr_t get_fip_hdr_page(void)
{
	return get_fip_hdr_base() & ~PAGE_MASK;
}

static mmap_region_t s32_mmap[] = {
	MAP_REGION_FLAT(S32_UART_BASE, S32_UART_SIZE,
			MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(SIUL2_0_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(SIUL2_0_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(SIUL2_1_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(SIUL2_1_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(I2C4_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(I2C4_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(OCOTP_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(OCOTP_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(SRAMC0_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(SRAMC_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(SRAMC1_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(SRAMC_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(FCCU_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(FCCU_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(DDRSS_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(DDRSS_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(GPR_BASE_PAGE_ADDR, MMU_ROUND_UP_TO_PAGE(GPR_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(MC_CGM5_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(MC_CGM5_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(S32_MC_RGM_BASE_ADDR, S32_MC_RGM_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(S32_MC_ME_BASE_ADDR, S32_MC_ME_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(DRAM_PLL_BASE_ADDR,
			MMU_ROUND_UP_TO_PAGE(DRAM_PLL_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(S32_FXOSC_BASE_ADDR,
			MMU_ROUND_UP_TO_PAGE(S32_FXOSC_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(USDHC_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(USDHC_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION2(S32_BL32_BASE, S32_BL32_BASE,
			MMU_ROUND_UP_TO_PAGE(S32_BL32_SIZE),
			MT_MEMORY | MT_RW, PAGE_SIZE),
	MAP_REGION2(BL33_DTB, BL33_DTB,
			MMU_ROUND_UP_TO_PAGE(S32_BL33_IMAGE_SIZE),
			MT_MEMORY | MT_RW, PAGE_SIZE),
	MAP_REGION_FLAT(S32_PMEM_START, S32_PMEM_LEN,
			MT_MEMORY | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32_OSPM_SCMI_MEM, S32_OSPM_SCMI_MEM_SIZE,
			MT_NON_CACHEABLE | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32_QSPI_BASE, S32_QSPI_SIZE, MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(S32_FLASH_BASE, FIP_MAXIMUM_SIZE, MT_RW | MT_SECURE),
	/* SCP entries */
	MAP_REGION_FLAT(MSCM_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(MSCM_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(S32_SCP_SCMI_MEM,
			MMU_ROUND_UP_TO_PAGE(S32_SCP_SCMI_MEM_SIZE),
			MT_NON_CACHEABLE | MT_RW | MT_SECURE),
#if defined(STM6_BASE_ADDR)
	MAP_REGION_FLAT(STM6_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(STM6_SIZE),
			MT_DEVICE | MT_RW),
#endif
#if (ERRATA_S32_050543 == 1)
	MAP_REGION_FLAT(DDR_ERRATA_REGION_BASE, DDR_ERRATA_REGION_SIZE,
			MT_NON_CACHEABLE | MT_RW),
#endif
#if defined(SRAMC2_BASE_ADDR)
	MAP_REGION_FLAT(SRAMC2_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(SRAMC_SIZE),
			MT_DEVICE | MT_RW),
#endif
#if defined(SRAMC3_BASE_ADDR)
	MAP_REGION_FLAT(SRAMC3_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(SRAMC_SIZE),
			MT_DEVICE | MT_RW),
#endif
#if defined(SSRAMC_BASE_ADDR)
	MAP_REGION_FLAT(SSRAMC_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(SRAMC_SIZE),
			MT_DEVICE | MT_RW),
#endif
#if defined(S32G_SSRAM_BASE)
	MAP_REGION_FLAT(S32G_SSRAM_BASE, S32G_SSRAM_LIMIT - S32G_SSRAM_BASE,
			 MT_MEMORY | MT_RW | MT_SECURE),
#endif
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

static bool filters_contain(uintptr_t base_addr,
			    const struct s32_mmu_filter *filters,
			    size_t n_filters)
{
	const struct s32_mmu_filter *filter;
	size_t i, j;

	for (i = 0; i < n_filters; i++) {
		filter = &filters[i];
		for (j = 0; j < filter->n_base_addrs; j++) {
			if (base_addr == filter->base_addrs[j])
				return true;
		}
	}

	return false;
}

static void filter_mmu_entries(const struct s32_mmu_filter *filters,
			       size_t n_filters)
{
	size_t i;
	bool used;

	for (i = 0; i < ARRAY_SIZE(s32_mmap); i++) {
		used = filters_contain(s32_mmap[i].base_pa, filters, n_filters);
		if (!used)
			s32_mmap[i].size = 0;
	}
}

static void get_fip_images_ranges(uintptr_t *images_base, size_t *images_size)
{
	uintptr_t fip_base = get_fip_mem_addr();
	uintptr_t fip_size, fip_end;

	fip_size = get_image_max_offset();

	if (fip_size < fip_base)
		panic();

	fip_size -= fip_base;

	/* No additional images are loaded into memory */
	if (!fip_base) {
		*images_base = 0;
		*images_size = 0;
		return;
	}

	assert(!check_uptr_overflow(fip_base, fip_size - 1));

	/* fip.bin is loaded into memory */
	fip_end = fip_base + fip_size;

	*images_base = BL2_END & ~PAGE_MASK;

	if (fip_end < *images_base)
		panic();

	*images_size = MMU_ROUND_UP_TO_PAGE(fip_end - *images_base);
}

int s32_el3_mmu_fixup(const struct s32_mmu_filter *filters, size_t n_filters)
{
	const unsigned long code_start = BL_CODE_BASE;
	const unsigned long rw_start = BL2_RW_START;
	unsigned long code_size;
	unsigned long rw_size;
	uintptr_t images_base = 0u;
	size_t images_size = 0u;

	if (BL_END < BL2_RW_START)
		return -EINVAL;

	if (BL_CODE_END < BL_CODE_BASE)
		return -EINVAL;

	code_size = BL_CODE_END - BL_CODE_BASE;
	rw_size = BL_END - BL2_RW_START;

	get_fip_images_ranges(&images_base, &images_size);

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
			.size = BL2_BASE - get_fip_hdr_page(),
			.attr = MT_RO | MT_MEMORY | MT_SECURE,
		},
		/* Additional images in case the FIP is loaded into memory */
		{
			.base_pa = images_base,
			.base_va = images_base,
			.size = images_size,
			.attr = MT_RO | MT_MEMORY | MT_SECURE,
		},
	};
	int i, ret;

	if (filters) {
		filter_mmu_entries(filters, n_filters);
	} else {
		ret = disable_qspi_mmu_entry();
		if (ret)
			return ret;
	}

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
	unsigned int op_mask = op & AARCH64_UNCOND_BRANCH_MASK;

	return op_mask == AARCH64_UNCOND_BRANCH_OP ||
		op_mask == AARCH64_UNCOND_BRANCH_OP_BOOT0_HOOK;
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

static int fdt_set_node_status(void *blob, int nodeoff, bool enable)
{
	const char *str;

	if (enable)
		str = "okay";
	else
		str = "disabled";

	return fdt_setprop_string(blob, nodeoff, "status", str);
}

static int disable_node_by_compatible(void *blob, const char *compatible,
				      uint32_t *phandle)
{
	const char *node_name;
	int nodeoff, ret;

	nodeoff = fdt_node_offset_by_compatible(blob, -1, compatible);
	if (nodeoff < 0) {
		ERROR("Failed to get a node based on compatible string '%s' (%s)\n",
		      compatible, fdt_strerror(nodeoff));
		return nodeoff;
	}

	node_name = fdt_get_name(blob, nodeoff, NULL);
	*phandle = fdt_get_phandle(blob, nodeoff);
	if (!*phandle) {
		ERROR("Failed to get phandle of '%s' node\n",
		      node_name);
		return *phandle;
	}

	ret = fdt_set_node_status(blob, nodeoff, false);
	if (ret) {
		ERROR("Failed to disable '%s' node (%s)\n",
		      node_name, fdt_strerror(ret));
		return ret;
	}

	ret = fdt_delprop(blob, nodeoff, "phandle");
	if (ret) {
		ERROR("Failed to remove phandle property of '%s' node: %s\n",
		       node_name, fdt_strerror(ret));
		return ret;
	}

	return 0;
}

static int set_scmi_protocol_node_status(void *blob, const char *path,
					 uint32_t phandle, bool enable)
{
	int nodeoff, ret;

	nodeoff = fdt_path_offset(blob, path);
	if (nodeoff < 0) {
		ERROR("Failed to get offset of '%s' node (%s)\n",
		      path, fdt_strerror(nodeoff));
		return nodeoff;
	}

	if (phandle) {
		ret = fdt_setprop_u32(blob, nodeoff, "phandle", phandle);
		if (ret) {
			ERROR("Failed to set phandle property of '%s' node (%s)\n",
			      path, fdt_strerror(ret));
			return ret;
		}
	}

	ret = fdt_set_node_status(blob, nodeoff, enable);
	if (ret) {
		ERROR("Failed to set status (%s) for node (%s)\n",
		      fdt_strerror(ret), path);
		return ret;
	}

	return 0;
}

static int enable_scmi_protocol(void *blob, const char *path, uint32_t phandle)
{
	return set_scmi_protocol_node_status(blob, path, phandle, true);
}

static int disable_siul2_gpio_node(void *blob, uint32_t *phandle)
{
	return disable_node_by_compatible(blob, "nxp,s32cc-siul2-gpio",
					  phandle);
}

static int enable_scmi_gpio_node(void *blob, uint32_t phandle)
{
	return enable_scmi_protocol(blob, gpio_scmi_node_path, phandle);
}

static int ft_fixup_gpio(void *blob)
{
	uint32_t phandle;
	int ret;

	ret = disable_siul2_gpio_node(blob, &phandle);
	if (ret)
		return ret;

	ret = enable_scmi_gpio_node(blob, phandle);
	if (ret)
		return ret;

	return 0;
}

static int find_nvmem_scmi_node(void *blob, int *nodeoff,
				const fdt32_t **phandles)
{
	int scmi_nvmem_nodeoff;
	const fdt32_t *scmi_nvmem_phandles;

	scmi_nvmem_nodeoff = fdt_path_offset(blob, nvmem_scmi_node_path);
	if (scmi_nvmem_nodeoff < 0) {
		ERROR("Failed to get NVMEM SCMI node with path '%s' (%s)\n",
		      nvmem_scmi_node_path, fdt_strerror(scmi_nvmem_nodeoff));
		return scmi_nvmem_nodeoff;
	}

	scmi_nvmem_phandles = fdt_getprop(blob, scmi_nvmem_nodeoff,
					  "nvmem-cells", NULL);
	if (!scmi_nvmem_phandles) {
		ERROR("Failed to get 'nvmem-cells' property of '%s' node\n",
		      nvmem_scmi_node_path);
		return -FDT_ERR_NOTFOUND;
	}

	*nodeoff = scmi_nvmem_nodeoff;
	*phandles = scmi_nvmem_phandles;

	return 0;
}

static int find_nvmem_consumer_node(void *blob, int nodeoff_scmi, int *nodeoff,
				    int *num_phandles)
{
	int count;
	int startoffset = *nodeoff;

	*nodeoff = fdt_node_offset_by_prop_found(blob, startoffset,
						 "nvmem-cells");
	/* Skip the NVMEM SCMI node */
	if (*nodeoff == nodeoff_scmi)
		*nodeoff = fdt_node_offset_by_prop_found(blob, *nodeoff,
							 "nvmem-cells");
	if (*nodeoff < 0) {
		if (startoffset == 0)
			ERROR("Failed to get at least 1 node with 'nvmem-cells' property (%s)\n",
			      fdt_strerror(*nodeoff));
		return -FDT_ERR_NOTFOUND;
	}

	/* Count string values in "nvmem-cell-names" property */
	count = fdt_stringlist_count(blob, *nodeoff, "nvmem-cell-names");
	if (count < 0) {
		ERROR("Failed to get 'nvmem-cell-names' property of node '%s' (%s)\n",
		      fdt_get_name(blob, *nodeoff, NULL),
		      fdt_strerror(count));
		return count;
	}
	if (count == 0) {
		ERROR("Empty 'nvmem-cell-names' property of node '%s'\n",
		      fdt_get_name(blob, *nodeoff, NULL));
		return -FDT_ERR_BADVALUE;
	}

	*num_phandles = count;

	return 0;
}

static int update_nvmem_consumer_phandles(void *blob, int nodeoff_consumer,
					  int num_phandles, int nodeoff_scmi,
					  const fdt32_t *phandles_scmi)
{
	int ret, i, idx;
	const char *cell_name;
	static uint32_t new_phandles[S32CC_SCMI_NVMEM_MAX];

	for (i = 0; i < num_phandles; i++) {
		/* Get nvmem cell name which corresponds to a phandle */
		cell_name = fdt_stringlist_get(blob, nodeoff_consumer,
					       "nvmem-cell-names", i, &ret);
		if (!cell_name) {
			ERROR("Failed to get cell name at index %d in node '%s' (%s)\n",
			      i, fdt_get_name(blob, nodeoff_consumer, NULL),
			      fdt_strerror(ret));
			return ret;
		}

		/* Find idx of 'cell_name' in nvmem node */
		idx = fdt_stringlist_search(blob, nodeoff_scmi,
					    "nvmem-cell-names", cell_name);
		if (idx < 0) {
			ERROR("Failed to get index of '%s' in node '%s' (%s)\n",
			      cell_name, nvmem_scmi_node_path,
			      fdt_strerror(idx));
			return idx;
		}

		/* Store new phandles to be written all at once */
		new_phandles[i] = phandles_scmi[idx];
	}

	ret = fdt_setprop_inplace(blob, nodeoff_consumer, "nvmem-cells",
				  new_phandles,
				  num_phandles * sizeof(uint32_t));
	if (ret) {
		ERROR("Failed to set 'nvmem-cells' property of '%s' node (%s)\n",
		      fdt_get_name(blob, nodeoff_consumer, NULL),
		      fdt_strerror(ret));
		return ret;
	}

	return 0;
}

static int ft_fixup_nvmem(void *blob)
{
	int nodeoff_scmi, nodeoff_consumer;
	int num_phandles, ret;
	const fdt32_t *phandles_scmi;

	ret = find_nvmem_scmi_node(blob, &nodeoff_scmi, &phandles_scmi);
	if (ret)
		return ret;

	/*
	 * Find all nodes with "nvmem-cells" property, except the
	 * SCMI NVMEM node
	 */
	nodeoff_consumer = 0;
	while (!(ret = find_nvmem_consumer_node(blob, nodeoff_scmi,
						&nodeoff_consumer,
						&num_phandles))) {
		/* Update node NVMEM phandles to point to NVMEM SCMI cells */
		ret = update_nvmem_consumer_phandles(blob, nodeoff_consumer,
						     num_phandles, nodeoff_scmi,
						     phandles_scmi);
		if (ret)
			return ret;
	}

	if (ret && nodeoff_consumer >= 0) {
		ERROR("Malformed node '%s' (%s)\n",
		      fdt_get_name(blob, nodeoff_consumer, NULL),
		      fdt_strerror(ret));
		return -EINVAL;
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

	if (is_scp_used() && is_gpio_scmi_fixup_enabled()) {
		ret = ft_fixup_gpio(blob);
		if (ret)
			goto out;
	}

	if (is_scp_used() && is_nvmem_scmi_fixup_enabled()) {
		ret = ft_fixup_nvmem(blob);
		if (ret)
			goto out;
	}

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
static void set_fip_images_size(void)
{
	static const uuid_t uuid_null = { {0} };
	uintptr_t fip_header = get_fip_hdr_base();
	fip_toc_header_t *toc_header = (fip_toc_header_t *)fip_header;
	uint8_t *buf_end = (uint8_t *)(fip_header + FIP_HEADER_SIZE);
	fip_toc_entry_t *toc_entry = (fip_toc_entry_t *)(toc_header + 1);

	while ((uint8_t *)toc_entry < buf_end) {
		if (compare_uuids(&toc_entry->uuid, &uuid_null) == 0)
			break;

		set_image_spec(&toc_entry->uuid, toc_entry->size,
			       toc_entry->offset_address);
		toc_entry++;
	}
}

void bl2_platform_setup(void)
{
	static bool executed;

	if (executed)
		return;

	set_fip_images_size();
	if (DEBUG)
		dump_images_spec();

	executed = true;
}

int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	uint32_t magic;
	int ret;

	bl_mem_params_node_t *bl_mem_params = NULL;
	bl_mem_params_node_t *pager_mem_params = NULL;
	bl_mem_params_node_t *paged_mem_params = NULL;

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

