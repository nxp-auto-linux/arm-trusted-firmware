/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <platform.h>
#include <common/bl_common.h>
#include <common/desc_image_load.h>
#include <common/debug.h>
#include <drivers/console.h>
#include <lib/mmio.h>
#include <lib/optee_utils.h>
#include "s32g_linflexuart.h"
#include "s32g_storage.h"
#include "s32g_mc_rgm.h"
#include "s32g_mc_me.h"
#include "bl31_ssram.h"
#include "s32g_lowlevel.h"
#include "s32g_bl_common.h"
#include <nxp/s32g/ddr/ddrss.h>
#include <drivers/generic_delay_timer.h>
#include <ssram_mailbox.h>
#include "s32g_sramc.h"
#include <lib/libfdt/libfdt.h>

#define S32G_FDT_UPDATES_SPACE		100U

#define AARCH64_UNCOND_BRANCH_MASK	(0x7c000000)
#define AARCH64_UNCOND_BRANCH_OP	(BIT(26) | BIT(28))
#define BL33_DTB_MAGIC			(0xedfe0dd0)

static bl_mem_params_node_t s32g_bl2_mem_params_descs[6];
REGISTER_BL_IMAGE_DESCS(s32g_bl2_mem_params_descs)

static void add_fip_img_to_mem_params_descs(bl_mem_params_node_t *params,
					    size_t *index)
{
	bl_mem_params_node_t node = {
		.image_id = FIP_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      NON_SECURE | EXECUTABLE),

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, IMAGE_ATTRIB_PLAT_SETUP),
		.image_info.image_max_size = FIP_MAXIMUM_SIZE,
		.image_info.image_base = FIP_BASE,
		.next_handoff_image_id = BL31_IMAGE_ID,
	};

	params[(*index)++] = node;
}

static void add_bl31_img_to_mem_params_descs(bl_mem_params_node_t *params,
					     size_t *index)
{
	bl_mem_params_node_t node = {
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

	params[(*index)++] = node;
}

#ifdef SPD_opteed
static void add_bl32_img_to_mem_params_descs(bl_mem_params_node_t *params,
					     size_t *index)
{
	bl_mem_params_node_t node = {
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

	params[(*index)++] = node;
}

static void add_bl32_extra1_img_to_mem_params_descs(
	bl_mem_params_node_t *params,
	size_t *index)
{
	bl_mem_params_node_t node = {

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

	params[(*index)++] = node;
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

void bl2_el3_early_platform_setup(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	size_t index;
	bl_mem_params_node_t *params = s32g_bl2_mem_params_descs;

	s32g_early_plat_init(false);
	s32g_io_setup();

	add_fip_img_to_mem_params_descs(params, &index);
	add_bl31_img_to_mem_params_descs(params, &index);
	add_bl32_img_to_mem_params_descs(params, &index);
	add_bl32_extra1_img_to_mem_params_descs(params, &index);
	add_bl33_img_to_mem_params_descs(params, &index);
	add_invalid_img_to_mem_params_descs(params, &index);

	bl_mem_params_desc_num = index;
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

static int ft_fixups(void *blob)
{
	size_t size = fdt_totalsize(blob);
	int ret;

	size += S32G_FDT_UPDATES_SPACE;
	fdt_set_totalsize(blob, size);

	ret = ft_fixup_scmi_clks(blob);

	flush_dcache_range((uintptr_t)blob, size);

	return ret;
}

static bool is_branch_op(uint32_t op)
{
	return (op & AARCH64_UNCOND_BRANCH_MASK) == AARCH64_UNCOND_BRANCH_OP;
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

void bl2_el3_plat_arch_setup(void)
{
	static struct console_s32g console;
	extern struct ddrss_conf ddrss_conf;
	extern struct ddrss_firmware ddrss_firmware;

	console_s32g_register(S32G_UART_BASE, S32G_UART_CLOCK_HZ,
			      S32G_UART_BAUDRATE, &console);

	s32g_sram_clear(S32G_BL33_IMAGE_BASE, DTB_BASE);
	s32g_ssram_clear();

	copy_bl31ssram_image();
	/* This will also populate CSR section from bl31ssram */
	ddrss_init(&ddrss_conf, &ddrss_firmware, BL31SSRAM_CSR_BASE);
}
