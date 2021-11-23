/*
 * Copyright 2019-2022 NXP
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
#include "s32_linflexuart.h"
#include "s32_storage.h"
#include "s32g_mc_rgm.h"
#include "s32g_mc_me.h"
#include "bl31_ssram.h"
#include "s32g_lowlevel.h"
#include "s32g_bl_common.h"
#include <drivers/generic_delay_timer.h>
#include <plat/nxp/s32g/bl31_ssram/ssram_mailbox.h>
#include "s32_sramc.h"
#include <lib/libc/errno.h>
#include <lib/libfdt/libfdt.h>
#include <drivers/io/io_storage.h>
#include <tools_share/firmware_image_package.h>
#include <s32_dt.h>
#if S32G_EMU == 1
#include <ddr/ddrss.h>
#else
#include <ddr/ddr_init.h>
#include <drivers/nxp/s32/ddr/ddr_lp.h>
#endif

#define S32G_FDT_UPDATES_SPACE		100U

#define AARCH64_UNCOND_BRANCH_MASK	(0x7c000000)
#define AARCH64_UNCOND_BRANCH_OP	(BIT(26) | BIT(28))
#define BL33_DTB_MAGIC			(0xedfe0dd0)

#define PER_GROUP3_BASE		(0x40300000UL)
#define FCCU_BASE_ADDR		(PER_GROUP3_BASE + 0x0000C000)
#define FCCU_NCF_S1			(FCCU_BASE_ADDR + 0x84)
#define FCCU_NCFK			(FCCU_BASE_ADDR + 0x90)
#define FCCU_NCFK_KEY		(0xAB3498FE)

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
		.image_info.image_max_size = S32_BL33_IMAGE_SIZE,
		.image_info.image_base = S32_BL33_IMAGE_BASE,
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

#if (ERRATA_S32G2_050543 == 1 && S32G_EMU == 0)
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

#if (ERRATA_S32G2_050543 == 1 && S32G_EMU == 0)
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

	while((char *)toc_entry < buf_end) {
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

/**
 * Clear non-critical faults generated by SWT (software watchdog timer)
 * All SWT faults are placed in NCF_S1 (33-38)
 */
static void clear_swt_faults(void)
{
	unsigned int val = mmio_read_32(FCCU_NCF_S1);

	if (val) {
		mmio_write_32(FCCU_NCFK, FCCU_NCFK_KEY);
		mmio_write_32(FCCU_NCF_S1, val);
	}
}

static void clear_reset_cause(void)
{
	uint32_t mc_rgm_des = mmio_read_32(MC_RGM_DES);

	mmio_write_32(MC_RGM_DES, mc_rgm_des);
}

static enum reset_cause get_reset_cause(void)
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

static const char *get_reset_cause_str(enum reset_cause reset_cause)
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

#if S32G_EMU == 0
static void resume_bl31(struct s32g_ssram_mailbox *ssram_mb)
{
	s32g_warm_entrypoint_t resume_entrypoint;
	uintptr_t csr_addr;

	resume_entrypoint = ssram_mb->bl31_warm_entrypoint;
	csr_addr = (uintptr_t)&ssram_mb->csr_settings[0];

	s32_enable_a53_clock();
	s32_enable_ddr_clock();
	ddrss_to_normal_mode(csr_addr);

	resume_entrypoint();
}
#endif

#define MMU_ROUND_UP_TO_4K(x)	\
			(((x) & ~0xfff) == (x) ? (x) : ((x) & ~0xfff) + 0x1000)

IMPORT_SYM(uintptr_t, __RW_START__, BL2_RW_START);

static mmap_region_t s32g_mmap[] = {
	MAP_REGION_FLAT(S32G_SSRAM_BASE, S32G_SSRAM_LIMIT - S32G_SSRAM_BASE,
			 MT_MEMORY | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32_UART_BASE, S32_UART_SIZE,
			MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(S32_MC_ME_BASE_ADDR, S32_MC_ME_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(MC_CGM0_BASE_ADDR,
			MMU_ROUND_UP_TO_4K(S32_DFS_ADDR(S32_DFS_NR)),
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
	MAP_REGION2(S32_BL33_IMAGE_BASE, S32_BL33_IMAGE_BASE,
			MMU_ROUND_UP_TO_4K(S32_BL33_IMAGE_SIZE),
			MT_MEMORY | MT_RW, PAGE_SIZE),
	MAP_REGION_FLAT(S32_PMEM_START, S32_PMEM_LEN,
			MT_MEMORY | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32G_SCMI_SHARED_MEM, S32G_SCMI_SHARED_MEM_SIZE,
			MT_NON_CACHEABLE | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32G_QSPI_BASE, S32G_QSPI_SIZE, MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(FIP_BASE, FIP_MAXIMUM_SIZE, MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32G_FLASH_BASE, FIP_MAXIMUM_SIZE, MT_RW | MT_SECURE),
	MAP_REGION_FLAT(DTB_BASE, BL2_BASE - DTB_BASE, MT_MEMORY | MT_RW),
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

	offset = fdt_node_offset_by_compatible(fdt, -1, "fsl,s32gen1-qspi");
	if (offset > 0) {
		if (fdt_get_status(offset) == DT_ENABLED)
			return 0;
	}

	for (i = 0; i < ARRAY_SIZE(s32g_mmap); i++) {
		if (s32g_mmap[i].base_pa == S32G_FLASH_BASE) {
			s32g_mmap[i].size = 0;
			break;
		}
	}

	return 0;
}

static int s32g_el3_mmu_fixup(void)
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
	int i, ret;

	ret = disable_qspi_mmu_entry();
	if (ret)
		return ret;

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

	return 0;
}

#if S32G_EMU == 1
static void skip_emu_images(bl_mem_params_node_t *params, size_t size)
{
	unsigned int image_id;
	size_t i;

	for (i = 0; i < size; i++) {
		image_id = params[i].image_id;

		if (image_id == BL31_IMAGE_ID || image_id == BL33_IMAGE_ID)
			params[i].image_info.h.attr |=
			    IMAGE_ATTRIB_SKIP_LOADING;
	}
}
#endif

void bl2_el3_early_platform_setup(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	enum reset_cause reset_cause;
	size_t index = 0;
	bl_mem_params_node_t *params = s32g_bl2_mem_params_descs;

	reset_cause = get_reset_cause();
	clear_reset_cause();

	/* No resume on emulator */
#if S32G_EMU == 0
	struct s32g_ssram_mailbox *ssram_mb = (void *)BL31SSRAM_MAILBOX;

	if ((reset_cause == CAUSE_WAKEUP_DURING_STANDBY) &&
	    !ssram_mb->short_boot) {
		/* Trampoline to bl31_warm_entrypoint */
		resume_bl31(ssram_mb);
		panic();
	}
#endif

	s32_early_plat_init(false);
	console_s32_register();
	s32_io_setup();

	NOTICE("Reset status: %s\n", get_reset_cause_str(reset_cause));

	add_fip_img_to_mem_params_descs(params, &index);
	add_bl31_img_to_mem_params_descs(params, &index);
	add_bl32_img_to_mem_params_descs(params, &index);
	add_bl32_extra1_img_to_mem_params_descs(params, &index);
	add_bl33_img_to_mem_params_descs(params, &index);
	add_invalid_img_to_mem_params_descs(params, &index);

#if S32G_EMU == 1
	skip_emu_images(params, index);
#endif

	bl_mem_params_desc_num = index;
}

#if S32G_EMU == 0
static void copy_bl31ssram_image(void)
{
	/* Copy bl31 ssram stage. This includes IVT */
	memcpy((void *)S32G_SSRAM_BASE, bl31ssram, bl31ssram_len);
}
#endif

void bl2_el3_plat_arch_setup(void)
{
	uint32_t ret;

#if S32G_EMU == 0
	ret = s32g_el3_mmu_fixup();
	if (ret)
		panic();

	dt_init_ocotp();
	dt_init_pmic();

	ret = pmic_setup();
	if (ret)
		ERROR("Failed to disable VR5510 watchdog\n");

	s32_sram_clear(S32_BL33_IMAGE_BASE, DTB_BASE);
	/* Clear only the necessary part for the FIP header. The rest will
	 * be cleared in bl2_plat_handle_post_image_load, before loading
	 * the entire FIP image.
	 */
	s32_sram_clear(FIP_BASE, FIP_BASE + FIP_HEADER_SIZE);

	s32_ssram_clear();

	copy_bl31ssram_image();

	clear_swt_faults();
#endif

	/* This will also populate CSR section from bl31ssram */
	ret = ddr_init();
	if (ret)
		panic();
}

#if S32G_EMU == 1
void bl2_plat_preload_setup(void)
{
	printf("Now it's time to load the following images:\n");
	printf("BL31 @ 0x%x\n", BL31_BASE);
	printf("U-Boot @ 0x%x\n", BL33_BASE);

	__asm__ volatile("b .");
}
#endif
