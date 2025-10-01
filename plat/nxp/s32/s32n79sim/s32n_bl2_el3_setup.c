/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <stdint.h>
#include <common/desc_image_load.h>
#include <common/fdt_wrappers.h>
#include <drivers/arm/pl011.h>
#include <drivers/generic_delay_timer.h>
#include <lib/mmio.h>
#include <lib/fconf/fconf.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <lib/libfdt/libfdt.h>
#include <lib/mmio.h>
#include <plat/common/platform.h>
#include <s32_bl_common.h>
#include <s32_bl2_common.h>
#include <s32n_bl_common.h>
#include <s32n_io_storage.h>
#include <s32n_fss_scmi.h>

#define S32N_FDT_UPDATES_SPACE		300U

#define MEMORY_STRING		"memory"

#define XRDC4_CCID_ENTRY_OFFSET		(0x30)
#define CCID_ENTRY_MASK				GENMASK_32(3, 0)

static bl_mem_params_node_t s32n_bl2_mem_params_descs[2];
REGISTER_BL_IMAGE_DESCS(s32n_bl2_mem_params_descs)
static console_t console;

static uint32_t get_cohort_id(void)
{
	static bool xrdc4_ccid_chi_mapped;
	int ret;

	if (!xrdc4_ccid_chi_mapped) {
		ret = mmap_add_dynamic_region(XRDC4_CCID_PERIPH_ADDR,
				XRDC4_CCID_PERIPH_ADDR,
				MMU_ROUND_UP_TO_PAGE(XRDC4_CCID_CHI_SIZE),
				MT_DEVICE | MT_RO | MT_SECURE);
		if (ret < 0) {
			ERROR("XRDC4_CCID_CHI mapping: error %d\n", ret);
			panic();
		}
		xrdc4_ccid_chi_mapped = true;
	}

	return mmio_read_32(XRDC4_CCID_PERIPH_ADDR + XRDC4_CCID_ENTRY_OFFSET)
		& CCID_ENTRY_MASK;
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

	bl33_img_limit = BL2_BASE - MMU_ROUND_UP_TO_PAGE(get_bl2_dtb_size()) - PAGE_SIZE - 1;
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
#if S32N_DDR_BOOT_SOURCE == 1
/*
 * Sets variable which represents the memory address
 * for an OS image and which is computed based on an
 * offset in memory.
 */
static int s32n_set_image_addr(void *blob, uint64_t offset, uint64_t *ramdisk_addr)
{
	unsigned long start = 0, size = 0;
	const char *node_name;
	int ret, offs = -1;

	if (!ramdisk_addr)
		return -EINVAL;

	offs = fdt_node_offset_by_prop_value(blob, offs, "device_type",
					     MEMORY_STRING, sizeof(MEMORY_STRING));
	if (offs < 0) {
		ERROR("Failed to get offset of '/memory' node (%s)\n",
		      fdt_strerror(offs));
		return offs;
	}

	node_name = fdt_get_name(blob, offs, NULL);
	ret = fdt_get_reg_props_by_index(blob, offs, 0, &start, &size);
	if (ret) {
		ERROR("Couldn't get 'reg' property values of node (%s)\n",
		      node_name);
		return ret;
	}

	*ramdisk_addr = start + offset;

	return 0;
}

static int ft_fixup_chosen_node(void *blob)
{
	uint32_t initrd_end_low, initrd_end_high, initrd_start_low, initrd_start_high;
	uint32_t ramdisk_size_addr, initrd_start_prop[2], initrd_end_prop[2];
	uint64_t ramdisk_size, initrd_end, ramdisk_addr;
	int ret, offs;

	ret = s32n_set_image_addr(blob, RAMDISK_RAM_OFFSET, &ramdisk_addr);
	if (ret) {
		ERROR("Failed to compute ramdisk address!\n");
		return ret;
	}

	ramdisk_size_addr = ramdisk_addr - 0x4;
	ramdisk_size = mmio_read_32(ramdisk_size_addr);
	initrd_end = ramdisk_addr + ramdisk_size;
	initrd_start_low = (uint32_t)(ramdisk_addr & GENMASK(31, 0));
	initrd_start_high = (uint32_t)(ramdisk_addr >> 32);
	initrd_end_low = (uint32_t)(initrd_end & GENMASK(31, 0));
	initrd_end_high = (uint32_t)(initrd_end >> 32);

	offs = fdt_path_offset(blob, "/chosen");
	if (offs < 0) {
		ERROR("Failed to get offset of '/chosen' node (%s)\n",
		      fdt_strerror(offs));
		return offs;
	}

	initrd_start_prop[0] = cpu_to_fdt32(initrd_start_high);
	initrd_start_prop[1] = cpu_to_fdt32(initrd_start_low);

	initrd_end_prop[0] = cpu_to_fdt32(initrd_end_high);
	initrd_end_prop[1] = cpu_to_fdt32(initrd_end_low);

	ret = fdt_setprop(blob, offs, "linux,initrd-start", initrd_start_prop,
			  sizeof(initrd_start_prop));
	if (ret < 0) {
		ERROR("Failed to set 'linux,initrd_start' property (%s)\n",
		      fdt_strerror(ret));
		return ret;
	}

	ret = fdt_setprop(blob, offs, "linux,initrd-end", initrd_end_prop,
			  sizeof(initrd_end_prop));
	if (ret < 0) {
		ERROR("Failed to set 'linux,initrd_end' property (%s)\n",
		      fdt_strerror(ret));
		return ret;
	}

	return 0;
}
#endif

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

int apply_bl2_fixups(void *blob)
{
	size_t size = fdt_totalsize(blob);
	int ret;

	size += S32N_FDT_UPDATES_SPACE;
	fdt_set_totalsize(blob, size);

	ret = ft_fixup_resmem_node(blob);
	if (ret)
		goto out;

#if S32N_LINUX_KERNEL_AS_BL33 == 1
#if S32N_DDR_BOOT_SOURCE == 1
	ret = ft_fixup_chosen_node(blob);
	if (ret)
		goto out;
#endif
	ret = ft_fixup_add_bootargs(blob);
#endif

out:
	flush_dcache_range((uintptr_t)blob, size);
	return ret;
}

int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	if (image_id == BL33_IMAGE_ID)
		return bl2_copy_bl31_dtb();

	return 0;
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
	uint32_t index = 0, cohort_id;
	int ret = 0;

	write_cntfrq_el0(plat_get_syscnt_freq2());
	generic_delay_timer_init();

	if (is_fss_used())
		fss_scmi_init();

	console_pl011_register(PL011_UART_BASE, PL011_UART_CLOCK_HZ,
		PL011_UART_BAUDRATE, &console);

	ret |= add_bl31_img_to_mem_params_descs(params, &index);
	ret |= add_bl33_img_to_mem_params_descs(params, &index);
	if (ret)
		panic();

	bl_mem_params_desc_num = index;

	NOTICE("BL2: welcome to S32N!\n");

	s32n_el3_mmu_init();

	if (is_fss_used()) {
		if (fss_display_reset_plat_reason())
			ERROR("Could not retrieve platform reset reason\n");

		cohort_id = get_cohort_id();
		if (cohort_id > 0) {
			if (fss_display_reset_cohort_reason(cohort_id))
				ERROR("Could not retrieve reset reason for Cohort %d\n",
				      cohort_id);
		} else {
			ERROR("Linux cannot run on Cohort %d, since it belongs to FSS\n",
			      cohort_id);
		}
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
