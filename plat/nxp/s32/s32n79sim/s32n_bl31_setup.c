/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <platform.h>
#include <common/bl_common.h>
#include <drivers/arm/gicv3.h>
#include <drivers/arm/arm_gicv3_common.h>
#include <drivers/arm/pl011.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <s32_bl_common.h>
#include <s32n_bl_common.h>
#include <platform_def.h>

/* To be provided by FSS */
#if (S32N_SIM == 1)
#define COUNTER_FREQUENCY       100000000 /* 100MHz */
#endif

static entry_point_info_t bl33_image_ep_info;
static console_t console;

void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
				u_register_t arg2, u_register_t arg3)
{
	(void)console_pl011_register(PL011_UART_BASE, PL011_UART_CLOCK_HZ,
		PL011_UART_BAUDRATE, &console);

	SET_PARAM_HEAD(&bl33_image_ep_info, PARAM_EP, VERSION_1, 0);
	bl33_image_ep_info.pc = BL33_ENTRYPOINT;
	bl33_image_ep_info.spsr = SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
	SET_SECURITY_STATE(bl33_image_ep_info.h.attr, NON_SECURE);

}

void bl31_plat_arch_setup(void)
{
	s32n_el3_mmu_init();
}

static unsigned int plat_s32n_mpidr_to_core_pos(unsigned long mpidr)
{
	return (unsigned int)plat_core_pos_by_mpidr(mpidr);
}

static uintptr_t rdistif_base_addrs[PLATFORM_CORE_COUNT];

static gicv3_driver_data_t s32n_gic_data = {
	.gicd_base = PLAT_GICD_BASE,
	.gicr_base = PLAT_GICR_BASE,
	.rdistif_num = PLATFORM_CORE_COUNT,
	.rdistif_base_addrs = rdistif_base_addrs,
	.interrupt_props = NULL,
	.interrupt_props_num = 0,
	.mpidr_to_core_pos = plat_s32n_mpidr_to_core_pos,
};

static void s32n_gic_setup(void)
{
	unsigned int pos = plat_my_core_pos();
	int ret;

	ret = mmap_add_dynamic_region(PLAT_GICD_BASE,
				      PLAT_GICD_BASE,
				      MMU_ROUND_UP_TO_PAGE(
					      PLAT_GICD_SIZE),
				      MT_DEVICE | MT_RW);
	if (ret < 0) {
		ERROR("GICD mapping: error %d\n", ret);
		panic();
	}

	ret = mmap_add_dynamic_region(PLAT_GICR_BASE,
				      PLAT_GICR_BASE,
				      MMU_ROUND_UP_TO_PAGE(
					      PLAT_GICR_SIZE),
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
	s32n_gic_setup();
}

void bl31_plat_runtime_setup(void)
{

}

struct entry_point_info *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	return &bl33_image_ep_info;
}

unsigned int plat_get_syscnt_freq2(void)
{
#if (S32N_SIM == 1)
	return COUNTER_FREQUENCY;
#else
	return 0;
#endif
}
