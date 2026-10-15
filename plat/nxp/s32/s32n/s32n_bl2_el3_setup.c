/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <common/desc_image_load.h>
#include <common/fdt_wrappers.h>
#include <drivers/arm/pl011.h>
#include <drivers/generic_delay_timer.h>
#include <lib/fconf/fconf.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <lib/libfdt/libfdt.h>
#include <drivers/nxp/s32n/s32n_clk.h>
#include <drivers/nxp/s32n/s32n_cohort_manager.h>
#include <dt-bindings/clock/s32n79-clock-freq.h>
#include <plat/common/platform.h>
#include <s32_bl_common.h>
#include <s32_bl2_common.h>
#include <s32n_bl_common.h>
#include <s32n_cohort.h>
#include <s32n_fss_tlv.h>
#include <s32n_io_storage.h>
#include <s32n_fss_scmi.h>
#include "s32n_plat_funcs.h"
#include "s32n_pl011.h"

#define S32N_FDT_UPDATES_SPACE		300U

#define MEMORY_STRING		"memory"

static bl_mem_params_node_t s32n_bl2_mem_params_descs[3];
REGISTER_BL_IMAGE_DESCS(s32n_bl2_mem_params_descs)

/*
 * The FSS SoC-info TLV (0xFF_F001) splits the derivative string into two
 * adjacent, NULL-terminated fields: a "product" part (first characters) and
 * a "family" part.
 */
static void s32n_display_soc_info(void)
{
	struct fss_soc_product prod = {0};
	struct fss_soc_family family = {0};
	char soc_name[32] = {0};
	size_t off = 0U;
	int ret;

	ret = fss_tlv_get_soc_info(&prod, &family);
	if (ret != 0) {
		NOTICE("BL2: welcome to S32N!\n");
		WARN("Could not retrieve SoC info from FSS TLV (ret=%d)\n", ret);
		return;
	}

	/* Build the full SoC name from the product and family fields. */
	if ((prod.name != NULL) && (prod.name_len < sizeof(soc_name))) {
		memcpy(soc_name, prod.name, prod.name_len);
		off = prod.name_len;
	}
	if ((family.name != NULL) &&
	    (family.name_len < (sizeof(soc_name) - off))) {
		memcpy(soc_name + off, family.name, family.name_len);
		off += family.name_len;
	}
	soc_name[off] = '\0';

	NOTICE("BL2: welcome to %s!\n", soc_name);
}

static int add_nt_fw_config_img_to_mem_params_descs(bl_mem_params_node_t *params, uint32_t *index)
{
	if (*index >= bl_mem_params_desc_num) {
		return -EINVAL;
	}

	params[(*index)++] = (bl_mem_params_node_t) {
		.image_id = NT_FW_CONFIG_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_IMAGE_BINARY,
			VERSION_2, entry_point_info_t,
			NON_SECURE | NON_EXECUTABLE),
		SET_STATIC_PARAM_HEAD(image_info, PARAM_IMAGE_BINARY,
			VERSION_2, image_info_t, 0),
		.next_handoff_image_id = BL31_IMAGE_ID,
		.image_info.image_max_size = get_max_bl33_dtb_size(),
		.image_info.image_base = get_bl33_dtb_base(),

	};

	return 0;
}

static int add_bl31_img_to_mem_params_descs(bl_mem_params_node_t *params, uint32_t *index)
{
	if (*index >= bl_mem_params_desc_num) {
		return -EINVAL;
	}

	params[(*index)++] = (bl_mem_params_node_t) {
		.image_id = BL31_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
			entry_point_info_t,
			SECURE | EXECUTABLE | EP_FIRST_EXE),
		.ep_info.pc = BL31_BASE,
		.ep_info.spsr = SPSR_64(MODE_EL3, MODE_SP_ELX,
			DISABLE_ALL_EXCEPTIONS),

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
			image_info_t, IMAGE_ATTRIB_PLAT_SETUP),
		.image_info.image_base = BL31_BASE,
		.image_info.image_max_size = BL31_SIZE,
		.next_handoff_image_id = BL33_IMAGE_ID,
	};

	return 0;
}

static int add_bl33_img_to_mem_params_descs(bl_mem_params_node_t *params, uint32_t *index)
{
	uintptr_t bl33_img_limit;

	if (*index >= bl_mem_params_desc_num) {
		return -EINVAL;
	}

	bl33_img_limit = BL2_BASE - 1;
	if (bl33_img_limit <= BL33_ENTRYPOINT) {
		ERROR("BL33 image limit <= BL33 entry point\n");
		panic();
	}

	params[(*index)++] = (bl_mem_params_node_t) {
		.image_id = BL33_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      NON_SECURE | EXECUTABLE),

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, 0),
		.image_info.image_base = BL33_ENTRYPOINT,
		.image_info.image_max_size = (bl33_img_limit - BL33_ENTRYPOINT + 1),
		.next_handoff_image_id = INVALID_IMAGE_ID,
	};

	return 0;
}

static struct image_info *s32n_get_image_info(unsigned int image_id)
{
	struct bl_mem_params_node *desc;

	desc = get_bl_mem_params_node(image_id);
	assert(desc);
	return &desc->image_info;
}

#if S32N_LINUX_KERNEL_AS_BL33 == 1
static int ft_fixup_add_bootargs(void *blob)
{
	int offs, ret;

	offs = fdt_path_offset(blob, "/chosen");
	if (offs < 0) {
		ERROR("Failed to get offset of '%s' node (%s)\n",
		      "/chosen", fdt_strerror(offs));
		return offs;
	}

	ret = fdt_setprop_string(blob, offs, "bootargs", S32N_CUSTOM_BOOTARGS);
	if (ret) {
		ERROR("Failed to set 'bootargs' (%s)!\n", fdt_strerror(ret));
		return ret;
	}

	return 0;
}
#endif

static inline bool reserve_bl_nodes_enabled(void)
{
	return S32N_DTS_RESERVE_BL_NODES;
}

static int ft_fixup_resmem_bl_nodes(void *blob)
{
	int ret;

	ret = fdt_add_reserved_memory(blob, "atf", BL31_BASE, BL31_SIZE);
	if (ret) {
		ERROR("Failed to add 'atf' /reserved-memory node");
		return ret;
	}

	ret = fdt_add_reserved_memory(blob, "bl2-resume", BL2_RESUME_BASE, BL2_RESUME_SIZE);
	if (ret) {
		ERROR("Failed to add 'bl2-resume' /reserved-memory node");
		return ret;
	}

	return 0;
}

static size_t increase_fdt_size(void *blob)
{
	size_t size = fdt_totalsize(blob);

	assert(size + S32N_FDT_UPDATES_SPACE < UINT32_MAX);

	size += S32N_FDT_UPDATES_SPACE;
	fdt_set_totalsize(blob, size);

	return size;
}

static int bl2_apply_dtb_fixups(void *blob)
{
	size_t size = 0;
	int ret = 0;

	if (reserve_bl_nodes_enabled()) {
		size = increase_fdt_size(blob);

		ret = ft_fixup_resmem_bl_nodes(blob);
		if (ret)
			goto out;
	}

#if S32N_LINUX_KERNEL_AS_BL33 == 1
	if (!size)
		size = increase_fdt_size(blob);

	ret = ft_fixup_add_bootargs(blob);
#endif

out:
	if (size)
		flush_dcache_range((uintptr_t)blob, size);

	return ret;
}

static void check_bl33_dtb_size(void *dtb)
{
	size_t dtb_size;

	dtb_size = fdt_totalsize(dtb);

	if (dtb_size > get_max_bl33_dtb_size()) {
		ERROR("The deployed device tree exceeds its allocated size. Please adjust BL33_DTB_SIZE accordingly.");
		panic();
	}
}

int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	int ret = 0;
	void *dtb;

	if (image_id == NT_FW_CONFIG_ID) {
		dtb = (void *)get_bl33_dtb_base();

		check_bl33_dtb_size(dtb);

		ret = bl2_apply_dtb_fixups(dtb);

		check_bl33_dtb_size(dtb);
	}

	return ret;
}

int bl2_plat_handle_pre_image_load(unsigned int image_id)
{
	struct image_info *image_info;

	image_info = s32n_get_image_info(image_id);

	return mmap_add_dynamic_region(image_info->image_base,
			image_info->image_base,
			MMU_ROUND_UP_TO_PAGE(image_info->image_max_size),
			MT_MEMORY | MT_RW | MT_SECURE);
}

void bl2_el3_plat_arch_setup(void)
{
}

void bl2_el3_early_platform_setup(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	bl_mem_params_node_t *params = s32n_bl2_mem_params_descs;
	uint32_t index = 0, cohort_id, rate;
	int ret = 0;

	write_cntfrq_el0(plat_get_syscnt_freq2());
	generic_delay_timer_init();

	if (s32n_smmu_config())
		panic();

	s32n_el3_mmu_init();
	fss_scmi_init_a2p();

	if (s32n_clk_init())
		panic();

	rate = get_pl011_clk_freq();
	if (!rate)
		panic();

	console_s32n_register(rate);

	ret |= add_nt_fw_config_img_to_mem_params_descs(params, &index);
	ret |= add_bl31_img_to_mem_params_descs(params, &index);
	ret |= add_bl33_img_to_mem_params_descs(params, &index);
	if (ret)
		panic();

	bl_mem_params_desc_num = index;

	s32n_display_soc_info();

	if (fss_display_reset_plat_reason())
		ERROR("Could not retrieve platform reset reason\n");

	if (s32n_get_cohort_id(&cohort_id)) {
		panic();
	}

	if (cohort_id > 0) {
		if (fss_display_reset_cohort_reason(cohort_id))
			ERROR("Could not retrieve reset reason for Cohort %d\n",
					cohort_id);
	} else {
		ERROR("Linux cannot run on Cohort %d, since it belongs to FSS\n",
				cohort_id);
	}

	if (LOG_LEVEL >= LOG_LEVEL_INFO) {
		if (fss_get_misc_info())
			ERROR("Could not retrieve platform firmware and device info.\n");
	}

	ret = s32n_cohort_manager_init(cohort_id);
	if (ret) {
		ERROR("Cohort Manager init failed (ret=%d)\n", ret);
		panic();
	}

	/* Open handles to a FIP image */
	plat_s32n_io_setup();
}

void bl2_platform_setup(void)
{
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
