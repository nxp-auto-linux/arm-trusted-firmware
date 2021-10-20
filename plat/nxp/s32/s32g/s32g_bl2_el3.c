/*
 * Copyright 2019-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <platform.h>
#include <common/bl_common.h>
#include <common/desc_image_load.h>
#include <common/debug.h>
#include <common/fdt_fixup.h>
#include <drivers/console.h>
#include <lib/mmio.h>
#include <lib/optee_utils.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include "s32g_clocks.h"
#include "s32g_linflexuart.h"
#include "s32g_storage.h"
#include "s32g_mc_rgm.h"
#include "s32g_mc_me.h"
#include "bl31_ssram.h"
#include "s32g_lowlevel.h"
#include "s32g_bl_common.h"
#include <ddr/ddr_init.h>
#include <drivers/generic_delay_timer.h>
#include <plat/nxp/s32g/bl31_ssram/ssram_mailbox.h>
#include "s32g_sramc.h"
#include <lib/libfdt/libfdt.h>
#include <drivers/io/io_storage.h>
#include <tools_share/firmware_image_package.h>
#include <drivers/nxp/s32g/ddr/ddr_lp.h>

#define S32G_FDT_UPDATES_SPACE		100U

#define AARCH64_UNCOND_BRANCH_MASK	(0x7c000000)
#define AARCH64_UNCOND_BRANCH_OP	(BIT(26) | BIT(28))
#define BL33_DTB_MAGIC			(0xedfe0dd0)

#define FIP_HEADER_SIZE			(0x200)

static bl_mem_params_node_t s32g_bl2_mem_params_descs[6];
REGISTER_BL_IMAGE_DESCS(s32g_bl2_mem_params_descs)

static void add_fip_img_to_mem_params_descs(bl_mem_params_node_t *params,
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

static void add_bl31_img_to_mem_params_descs(bl_mem_params_node_t *params,
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
static void add_bl32_img_to_mem_params_descs(bl_mem_params_node_t *params,
					     size_t *index)
{
	params[(*index)++] = (bl_mem_params_node_t) {
		.image_id = BL32_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      SECURE | EXECUTABLE),
		.ep_info.pc = S32G_BL32_BASE,

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, 0),
		.image_info.image_max_size = S32G_BL32_SIZE,
		.image_info.image_base = S32G_BL32_BASE,
		.next_handoff_image_id = BL33_IMAGE_ID,
	};
}

static void add_bl32_extra1_img_to_mem_params_descs(
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
		.image_info.image_base = S32G_BL32_BASE,
		.image_info.image_max_size = S32G_BL32_SIZE,

		.next_handoff_image_id = INVALID_IMAGE_ID,
	};
}

#else
static void add_bl32_img_to_mem_params_descs(bl_mem_params_node_t *params,
					     size_t *index)
{

}

static void add_bl32_extra1_img_to_mem_params_descs(
	bl_mem_params_node_t *params,
	size_t *index)
{

}
#endif /* SPD_opteed */

static void add_bl33_img_to_mem_params_descs(bl_mem_params_node_t *params,
					     size_t *index)
{
	bl_mem_params_node_t node = {
		.image_id = BL33_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      NON_SECURE | EXECUTABLE),

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, 0),
		.image_info.image_max_size = S32G_BL33_IMAGE_SIZE,
		.image_info.image_base = S32G_BL33_IMAGE_BASE,
		.next_handoff_image_id = INVALID_IMAGE_ID,
	};

	params[(*index)++] = node;
}

static void add_invalid_img_to_mem_params_descs(bl_mem_params_node_t *params,
						size_t *index)
{
	bl_mem_params_node_t node = {
		.image_id = INVALID_IMAGE_ID,
		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, IMAGE_ATTRIB_SKIP_LOADING),
	};

	params[(*index)++] = node;
}

void bl2_platform_setup(void)
{
	return;
}

struct bl_params *plat_get_next_bl_params(void)
{
	return get_next_bl_params_from_mem_params_desc();
}

void plat_flush_next_bl_params(void)
{
	flush_bl_params_desc();
}

struct bl_load_info *plat_get_bl_image_load_info(void)
{
	return get_bl_load_info_from_mem_params_desc();
}

static int disable_clk_node(void *blob, uint32_t *phandle)
{
	const char *clk_path;
	int nodeoff, ret;

	clk_path = fdt_get_alias(blob, "clks");
	if (!clk_path) {
		ERROR("Failed to get the path of 'clks' alias\n");
		return -EIO;
	}

	nodeoff = fdt_path_offset(blob, clk_path);
	if (nodeoff < 0) {
		ERROR("Failed to get offset of '%s' node\n", clk_path);
		return nodeoff;
	}

	*phandle = fdt_get_phandle(blob, nodeoff);
	if (*phandle < 0) {
		ERROR("Failed to get phandle of '%s' node\n", clk_path);
		return *phandle;
	}

	ret = fdt_setprop_string(blob, nodeoff, "status", "disabled");
	if (ret) {
		ERROR("Failed to disable '%s' node\n", clk_path);
		return ret;
	}

	ret = fdt_delprop(blob, nodeoff, "phandle");
	if (ret) {
		ERROR("Failed to remove phandle property of '%s' node\n",
		       clk_path);
		return ret;
	}

	return 0;
}

static int enable_scmi_clk_node(void *blob, uint32_t phandle)
{
	int nodeoff, ret;

	nodeoff = fdt_path_offset(blob, "/firmware/scmi/protocol@14");
	if (nodeoff < 0) {
		ERROR("Failed to get offset of '/firmware/scmi/protocol@14' node\n");
		return nodeoff;
	}

	ret = fdt_setprop_cell(blob, nodeoff, "phandle", phandle);
	if (ret) {
		ERROR("Failed to set phandle property of '/firmware/scmi/protocol@14' node\n");
		return ret;
	}

	ret = fdt_setprop_string(blob, nodeoff, "status", "okay");
	if (ret) {
		ERROR("Failed to enable '/firmware/scmi/protocol@14' node\n");
		return ret;
	}

	return 0;
}

static int enable_scmi_mbox(void *blob)
{
	int nodeoff, ret;

	nodeoff = fdt_node_offset_by_compatible(blob, -1, "arm,scmi-smc");
	if (nodeoff < 0) {
		ERROR("Failed to get offset of 'arm,scmi-smc' node\n");
		return nodeoff;
	}

	ret = fdt_setprop_string(blob, nodeoff, "status", "okay");
	if (ret) {
		ERROR("Failed to enable 'arm,scmi-smc' node\n");
		return nodeoff;
	}

	return 0;
}

static int ft_fixup_scmi_clks(void *blob)
{
	uint32_t phandle;
	int ret;

	ret = disable_clk_node(blob, &phandle);
	if (ret)
		return ret;

	ret = enable_scmi_clk_node(blob, phandle);
	if (ret)
		return ret;

	ret = enable_scmi_mbox(blob);
	if (ret)
		return ret;

	return 0;
}

#if (ERRATA_S32G2_050543 == 1)
static int ft_fixup_ddr_errata(void *blob)
{
	int nodeoff, ret;

	if (polling_needed != 1) {
		return 0;
	}

	nodeoff = fdt_node_offset_by_compatible(blob, -1, "fsl,s32gen1-ddr-err050543");
	if (nodeoff < 0) {
		ERROR("Failed to get offset of 'fsl,s32gen1-ddr-err050543' node\n");
		return nodeoff;
	}

	ret = fdt_setprop_string(blob, nodeoff, "status", "okay");
	if (ret) {
		ERROR("Failed to enable 'fsl,s32gen1-ddr-err050543' node\n");
		return ret;
	}

	return 0;
}
#endif

static int ft_fixup_resmem_node(void *blob)
{
	int ret;
	char nodename[21];

	snprintf(nodename, sizeof(nodename), "atf@%x", BL31_BASE);

	ret = fdt_add_reserved_memory(blob, nodename, BL31_BASE, BL31_SIZE);
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

	size += S32G_FDT_UPDATES_SPACE;
	fdt_set_totalsize(blob, size);

	ret = ft_fixup_scmi_clks(blob);
	if (ret)
		goto out;

#if (ERRATA_S32G2_050543 == 1)
	ret = ft_fixup_ddr_errata(blob);
	if (ret)
		goto out;
#endif

	ret = ft_fixup_resmem_node(blob);

out:
	flush_dcache_range((uintptr_t)blob, size);
	return ret;
}

static bool is_branch_op(uint32_t op)
{
	return (op & AARCH64_UNCOND_BRANCH_MASK) == AARCH64_UNCOND_BRANCH_OP;
}

/* Return 0 for equal uuids. */
static inline int compare_uuids(const uuid_t *uuid1, const uuid_t *uuid2)
{
	return memcmp(uuid1, uuid2, sizeof(uuid_t));
}

/* Computes the real FIP image size and updates image info.
 * At this point, only FIP header was read so we can walk though all
 * fip_toc_entry_t entries until the last one.
 * The last entry will give us FIP image size.
 */
static int set_fip_size(bl_mem_params_node_t *fip_params)
{
	static const uuid_t uuid_null = { {0} };
	uint64_t last_offset = 0, last_size = 0;
	image_info_t *image_info = &fip_params->image_info;
	char *buf = (char *)image_info->image_base;
	char *buf_end = buf + image_info->image_size;
	fip_toc_header_t *toc_header = (fip_toc_header_t *)buf;
	fip_toc_entry_t *toc_entry = (fip_toc_entry_t *)(toc_header + 1);

	while((char *)toc_entry < buf_end) {
		if (compare_uuids(&toc_entry->uuid, &uuid_null) == 0)
			break;

		last_offset = toc_entry->offset_address;
		last_size = toc_entry->size;

		toc_entry++;
	}

	/* Update the real image size. */
	image_info->image_size = last_size + last_offset;

	return 0;
}

int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	uint32_t magic;
	int ret;

	bl_mem_params_node_t *bl_mem_params = NULL;
	bl_mem_params_node_t *pager_mem_params = NULL;
	bl_mem_params_node_t *paged_mem_params = NULL;

#define AARCH64_UNCOND_BRANCH_MASK	(0x7c000000)
#define AARCH64_UNCOND_BRANCH_OP	(BIT(26) | BIT(28))
#define BL33_DTB_MAGIC			(0xedfe0dd0)

	if (image_id == FIP_IMAGE_ID) {
		bl_mem_params = get_bl_mem_params_node(image_id);
		assert(bl_mem_params && "FIP params cannot be NULL");

		set_fip_size(bl_mem_params);

		/* Now that we know the real image size, we can load
		 * the entire FIP.
		 */
		s32g_sram_clear(FIP_BASE, FIP_BASE + bl_mem_params->image_info.image_size);
		ret = load_auth_image(image_id, &bl_mem_params->image_info);
		if (ret != 0) {
			ERROR("BL2: Failed to load image id %d (%i)\n",
			      image_id, ret);
			plat_error_handler(ret);
		}
	}

	if (image_id == BL33_IMAGE_ID) {
		magic = mmio_read_32(BL33_ENTRYPOINT);
		if (!is_branch_op(magic))
			printf("Warning: Instruction at BL33_ENTRYPOINT"
			       " is 0x%x, which is not a B or BL!\n",
			       magic);

		magic = mmio_read_32(BL33_DTB);
		if (magic != BL33_DTB_MAGIC) {
			printf("Error: Instruction at BL33_DTB is 0x%x"
			       ", which is not the expected 0x%x!\n",
			       magic, BL33_DTB_MAGIC);
			return -EINVAL;
		}

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

enum reset_cause get_reset_cause(void)
{
	uint32_t mc_rgm_des = mmio_read_32(MC_RGM_DES);

	if (mc_rgm_des & DES_F_POR)
		return CAUSE_POR;

	if (mc_rgm_des & DES_F_DR_ANY) {
		if (mmio_read_32(MC_RGM_RDSS) & RDSS_DES_RES)
			return CAUSE_DESTRUCTIVE_RESET_DURING_STANDBY;
		else
			return CAUSE_DESTRUCTIVE_RESET_DURING_RUN;
	}

	if (mmio_read_32(MC_RGM_FES) & FES_F_FR_ANY) {
		if (mmio_read_32(MC_RGM_RDSS) & RDSS_FES_RES)
			return CAUSE_FUNCTIONAL_RESET_DURING_STANDBY;
		else
			return CAUSE_FUNCTIONAL_RESET_DURING_RUN;
	}

	if (mmio_read_32(MC_ME_MODE_STAT) & MODE_STAT_PREV_MODE)
		return CAUSE_WAKEUP_DURING_STANDBY;

	return CAUSE_ERROR;
}

static void copy_bl31ssram_image(void)
{
	/* Copy bl31 ssram stage. This includes IVT */
	memcpy((void *)S32G_SSRAM_BASE, bl31ssram, bl31ssram_len);
}

static void resume_bl31(struct s32g_ssram_mailbox *ssram_mb)
{
	s32g_warm_entrypoint_t resume_entrypoint;
	uintptr_t csr_addr;

	resume_entrypoint = ssram_mb->bl31_warm_entrypoint;
	csr_addr = (uintptr_t)&ssram_mb->csr_settings[0];

	s32g_enable_a53_clock();
	s32g_enable_ddr_clock();
	ddrss_to_normal_mode(csr_addr);

	resume_entrypoint();
}

#define MMU_ROUND_UP_TO_4K(x)	\
			(((x) & ~0xfff) == (x) ? (x) : ((x) & ~0xfff) + 0x1000)

IMPORT_SYM(uintptr_t, __RW_START__, BL2_RW_START);

static const mmap_region_t s32g_mmap[] = {
	MAP_REGION_FLAT(S32G_SSRAM_BASE, S32G_SSRAM_LIMIT - S32G_SSRAM_BASE,
			 MT_MEMORY | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32G_UART_BASE, S32G_UART_SIZE,
			MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(S32G_MC_ME_BASE_ADDR, S32G_MC_ME_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(MC_CGM0_BASE_ADDR,
			MMU_ROUND_UP_TO_4K(S32G_DFS_ADDR(S32G_DFS_NR)),
			MT_DEVICE | MT_RW),
	/* This will cover both the MC_RGM and the GPR accesses, while reducing
	 * the number of used up MMU regions.
	 */
	MAP_REGION_FLAT(S32G_MC_RGM_BASE_ADDR, S32G_MC_RGM_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(SRAMC0_BASE_ADDR, SRAMC_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(SSRAMC_BASE_ADDR, SRAMC_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION2(S32G_BL32_BASE, S32G_BL32_BASE,
			MMU_ROUND_UP_TO_4K(S32G_BL32_SIZE),
			MT_MEMORY | MT_RW, PAGE_SIZE),
	MAP_REGION2(S32G_BL33_IMAGE_BASE, S32G_BL33_IMAGE_BASE,
			MMU_ROUND_UP_TO_4K(S32G_BL33_IMAGE_SIZE),
			MT_MEMORY | MT_RW, PAGE_SIZE),
	MAP_REGION_FLAT(S32G_PMEM_START, S32G_PMEM_LEN,
			MT_MEMORY | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32G_SCMI_SHARED_MEM, S32G_SCMI_SHARED_MEM_SIZE,
			MT_NON_CACHEABLE | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32G_QSPI_BASE, S32G_QSPI_SIZE, MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(FIP_BASE, FIP_MAXIMUM_SIZE, MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32G_FLASH_BASE, FIP_MAXIMUM_SIZE, MT_RW | MT_SECURE),
	MAP_REGION_FLAT(DTB_BASE, BL2_BASE - DTB_BASE, MT_MEMORY | MT_RW),
	{0},
};

void s32g_el3_mmu_fixup(void)
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
	};
	int i;

	/* Check the BL31/BL32/BL33 memory ranges for overlapping */
	_Static_assert(S32G_BL32_BASE + S32G_BL32_SIZE <= BL31_BASE,
				"BL32 and BL31 memory ranges overlap!");
	_Static_assert(BL31_BASE + BL31_SIZE <= BL33_BASE,
				"BL31 and BL33 memory ranges overlap!");

	/* The calls to mmap_add_region() consume mmap regions,
	 * so they must be counted in the static asserts
	 */
	_Static_assert(ARRAY_SIZE(s32g_mmap) + ARRAY_SIZE(regions) - 1 <=
		       MAX_MMAP_REGIONS,
		       "Fewer MAX_MMAP_REGIONS than in s32g_mmap will likely "
		       "result in a MMU exception at runtime");
	_Static_assert(ARRAY_SIZE(s32g_mmap) + ARRAY_SIZE(regions) - 1 +
		       BL31SRAM_MAX_PAGES <= MAX_XLAT_TABLES,
		       "Fewer MAX_XLAT_TABLES than in s32g_mmap will likely "
		       "result in a MMU exception at runtime");

	/* MMU initialization; while technically not necessary, improves
	 * bl2_load_images execution time.
	 */
	for (i = 0; i < ARRAY_SIZE(regions); i++)
		mmap_add_region(regions[i].base_pa, regions[i].base_va,
				regions[i].size, regions[i].attr);

	mmap_add(s32g_mmap);

	init_xlat_tables();
	enable_mmu_el3(0);
}

void bl2_el3_early_platform_setup(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	size_t index;
	bl_mem_params_node_t *params = s32g_bl2_mem_params_descs;
	struct s32g_ssram_mailbox *ssram_mb = (void *)BL31SSRAM_MAILBOX;

	if ((get_reset_cause() == CAUSE_WAKEUP_DURING_STANDBY) &&
	    !ssram_mb->short_boot) {
		/* Trampoline to bl31_warm_entrypoint */
		resume_bl31(ssram_mb);
		panic();
	}

	s32g_early_plat_init(false);
	console_s32g_register();
	s32g_io_setup();

	add_fip_img_to_mem_params_descs(params, &index);
	add_bl31_img_to_mem_params_descs(params, &index);
	add_bl32_img_to_mem_params_descs(params, &index);
	add_bl32_extra1_img_to_mem_params_descs(params, &index);
	add_bl33_img_to_mem_params_descs(params, &index);
	add_invalid_img_to_mem_params_descs(params, &index);

	bl_mem_params_desc_num = index;
}

void bl2_el3_plat_arch_setup(void)
{
	uint32_t ret;

	s32g_el3_mmu_fixup();

	dt_init_ocotp();
	dt_init_pmic();

	ret = pmic_setup();
	if (ret)
		ERROR("Failed to disable VR5510 watchdog\n");

	s32g_sram_clear(S32G_BL33_IMAGE_BASE, DTB_BASE);
	/* Clear only the necessary part for the FIP header. The rest will
	 * be cleared in bl2_plat_handle_post_image_load, before loading
	 * the entire FIP image.
	 */
	s32g_sram_clear(FIP_BASE, FIP_BASE + FIP_HEADER_SIZE);

	s32g_ssram_clear();

	copy_bl31ssram_image();
	/* This will also populate CSR section from bl31ssram */
	ret = ddr_init();
	if (ret)
		panic();
}
