/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <arch/aarch64/arch_features.h>
#include <common/bl_common.h>
#include <drivers/arm/gicv3.h>
#include <drivers/arm/arm_gicv3_common.h>
#include <drivers/arm/pl011.h>
#include <drivers/generic_delay_timer.h>
#include <dt-bindings/clock/s32n79-clock-freq.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <s32_bl_common.h>
#include <s32n_bl_common.h>
#include <s32n_fconf_gic_getter.h>
#include <s32n_fss_scmi.h>
#include <plat/arm/common/plat_arm.h>
#include <platform.h>
#include <platform_def.h>

static entry_point_info_t bl33_image_ep_info;
static console_t console;

extern uint8_t jump_to_bl31[];
extern uint8_t jump_to_bl31_end[];

static void s32n_plat_set_warm_entry(void)
{
	size_t size = jump_to_bl31_end - jump_to_bl31;

	/* update cold boot entrypoint with custom resume function */
	memcpy((void *)BL2_RESUME_BASE, (void *)jump_to_bl31, size);
}

void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
				u_register_t arg2, u_register_t arg3)
{
	(void)console_pl011_register(PL011_UART_BASE,
			GR_CLK_COSS_PERIPH_DIV0_RATE, PL011_UART_BAUDRATE,
			&console);

	SET_PARAM_HEAD(&bl33_image_ep_info, PARAM_EP, VERSION_1, 0);
	bl33_image_ep_info.pc = plat_get_ns_image_entrypoint();
	bl33_image_ep_info.spsr = arm_get_spsr_for_bl33_entry();
	bl33_image_ep_info.args.arg0 = get_bl33_dtb_base();
	SET_SECURITY_STATE(bl33_image_ep_info.h.attr, NON_SECURE);

}

void bl31_plat_arch_setup(void)
{
	s32n_plat_set_warm_entry();

	s32n_el3_mmu_init();
	generic_delay_timer_init();

	fss_scmi_init_a2p();
	fss_scmi_init_p2a();
}

static unsigned int plat_s32n_mpidr_to_core_pos(unsigned long mpidr)
{
	return (unsigned int)plat_core_pos_by_mpidr(mpidr);
}

static uintptr_t rdistif_base_addrs[PLATFORM_CORE_COUNT];

static gicv3_driver_data_t s32n_gic_data = {
	.rdistif_num = PLATFORM_CORE_COUNT,
	.rdistif_base_addrs = rdistif_base_addrs,
	.interrupt_props = NULL,
	.interrupt_props_num = 0,
	.mpidr_to_core_pos = plat_s32n_mpidr_to_core_pos,
};

static void s32n_gic_setup(void)
{
	unsigned int pos = plat_my_core_pos();
	size_t size;
	int ret;

	s32n_gic_data.gicd_base =
		FCONF_GET_PROPERTY(hw_config, gic_config, gicd_base);
	size = FCONF_GET_PROPERTY(hw_config, gic_config, gicd_size);

	if (!s32n_gic_data.gicd_base || !size) {
		ERROR("No GICD info in device tree\n");
		panic();
	}

	ret = mmap_add_dynamic_region(s32n_gic_data.gicd_base,
					s32n_gic_data.gicd_base,
					MMU_ROUND_UP_TO_PAGE(size),
					MT_DEVICE | MT_RW);
	if (ret < 0) {
		ERROR("GICD mapping: error %d\n", ret);
		panic();
	}

	s32n_gic_data.gicr_base =
		FCONF_GET_PROPERTY(hw_config, gic_config, gicr_base);
	size = FCONF_GET_PROPERTY(hw_config, gic_config, gicr_size);

	if (!s32n_gic_data.gicr_base || !size) {
		ERROR("No GICR info in device tree\n");
		panic();
	}

	ret = mmap_add_dynamic_region(s32n_gic_data.gicr_base,
				    s32n_gic_data.gicr_base,
				    MMU_ROUND_UP_TO_PAGE(size),
				    MT_DEVICE | MT_RW);
	if (ret < 0) {
		ERROR("GICR mapping: error %d\n", ret);
		panic();
	}

	gicv3_driver_init(&s32n_gic_data);
	gicv3_distif_init();
	gicv3_rdistif_init(pos);
	gicv3_cpuif_enable(pos);
}

void bl31_platform_setup(void)
{
	if (LOG_LEVEL >= LOG_LEVEL_INFO) {
		if (fss_scmi_nxp_core_validate()) {
			ERROR("SCMI NXP Core protocol initialization failed.\n");
			panic();
		}

		if (fss_get_misc_info())
			ERROR("Could not retrieve platform firmware and device info.\n");
	}

	s32n_gic_setup();
}

void bl31_plat_runtime_setup(void)
{

}

struct entry_point_info *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	return &bl33_image_ep_info;
}
