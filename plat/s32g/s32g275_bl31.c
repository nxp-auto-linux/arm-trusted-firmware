/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <common/bl_common.h>
#include <psci.h>
#include <drivers/arm/gicv3.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <lib/mmio.h>

#include "platform_def.h"
#include "s32g_mc_me.h"
#include "s32g_mc_rgm.h"
#include "s32g_linflexuart.h"
#include "s32g_lowlevel.h"
#include "s32g_xrdc.h"
#include "s32g_clocks.h"
#include "s32g_pinctrl.h"

IMPORT_SYM(uintptr_t, __RW_START__, BL31_RW_START);
IMPORT_SYM(uintptr_t, __RW_END__, BL31_RW_END);

static const mmap_region_t s32g_mmap[] = {
	MAP_REGION_FLAT(S32G_UART_BASE, S32G_UART_SIZE,
			MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(S32G275_GIC_BASE, S32G275_GIC_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(S32G_XRDC_BASE, S32G_XRDC_SIZE,
			MT_DEVICE | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32G_MC_ME_BASE_ADDR, S32G_MC_ME_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(S32G_MC_RGM_BASE_ADDR, S32G_MC_RGM_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(S32G_BL33_IMAGE_BASE, S32G_BL33_IMAGE_SIZE,
			MT_MEMORY | MT_RW),
	MAP_REGION_FLAT(S32G_PMEM_START, S32G_PMEM_LEN,
			MT_MEMORY | MT_RW | MT_SECURE),
	{0},
};

static entry_point_info_t bl33_image_ep_info;

static uintptr_t rdistif_base_addrs[PLATFORM_CORE_COUNT];

static const interrupt_prop_t interrupt_props[] = {
	INTR_PROP_DESC(S32G_SECONDARY_WAKE_SGI, GIC_HIGHEST_SEC_PRIORITY,
		       INTR_GROUP0, GIC_INTR_CFG_EDGE),
};

static unsigned int plat_s32g275_mpidr_to_core_pos(unsigned long mpidr);
/* Declare it here to avoid including plat/common/platform.h */
unsigned int plat_my_core_pos(void);

const gicv3_driver_data_t s32g275_gic_data = {
	.gicd_base = PLAT_GICD_BASE,
	.gicr_base = PLAT_GICR_BASE,
	.rdistif_num = PLATFORM_CORE_COUNT,
	.rdistif_base_addrs = rdistif_base_addrs,
	.interrupt_props = interrupt_props,
	.interrupt_props_num = ARRAY_SIZE(interrupt_props),
	.mpidr_to_core_pos = plat_s32g275_mpidr_to_core_pos,
};


static uint32_t s32g_get_spsr_for_bl33_entry(void)
{
	uint32_t spsr;
	unsigned long el_status, mode;
	unsigned int dbg_current_el;

	/* xDBGx print current EL */
	dbg_current_el = get_current_el();
	printf("Current EL is %u\n", dbg_current_el);

	/* figure out what mode we enter the non-secure world */
	el_status = read_id_aa64pfr0_el1() >> ID_AA64PFR0_EL2_SHIFT;
	el_status &= ID_AA64PFR0_ELX_MASK;

	mode = (el_status) ? MODE_EL2 : MODE_EL1;
	assert(mode == MODE_EL2);

	spsr = SPSR_64(mode, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);

	return spsr;
}

entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	assert(sec_state_is_valid(type));

	return &bl33_image_ep_info;
}

void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
		u_register_t arg2, u_register_t arg3)
{
#if RESET_TO_BL31
	assert((void *)arg0 == NULL); /* from bl2 */
	assert((void *)arg1 == NULL); /* plat params from bl2 */
#endif

	SET_PARAM_HEAD(&bl33_image_ep_info, PARAM_EP, VERSION_1, 0);
	bl33_image_ep_info.pc = S32G_BL33_IMAGE_BASE;
	bl33_image_ep_info.spsr = s32g_get_spsr_for_bl33_entry();
	SET_SECURITY_STATE(bl33_image_ep_info.h.attr, NON_SECURE);
}

static void s32g_el3_mmu_fixup(void)
{
	const unsigned long code_start = BL_CODE_BASE;
	const unsigned long code_size = BL_CODE_END - BL_CODE_BASE;
	const unsigned long rw_start = BL31_RW_START;
	const unsigned long rw_size = BL31_RW_END - BL31_RW_START;
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

	/* The calls to mmap_add_region() consume mmap regions,
	 * so they must be counted in the static asserts
	 */
	_Static_assert(ARRAY_SIZE(s32g_mmap) + ARRAY_SIZE(regions) - 1 <=
		       MAX_MMAP_REGIONS,
		       "Fewer MAX_MMAP_REGIONS than in s32g_mmap will likely "
		       "result in a MMU exception at runtime");
	_Static_assert(ARRAY_SIZE(s32g_mmap) + ARRAY_SIZE(regions) - 1 <=
		       MAX_XLAT_TABLES,
		       "Fewer MAX_XLAT_TABLES than in s32g_mmap will likely "
		       "result in a MMU exception at runtime");
	/* MMU initialization; while technically not necessary on cold boot,
	 * it is required for warm boot path processing
	 */
	for (i = 0; i < ARRAY_SIZE(regions); i++)
		mmap_add_region(regions[i].base_pa, regions[i].base_va,
				regions[i].size, regions[i].attr);

	mmap_add(s32g_mmap);

	init_xlat_tables();
	enable_mmu_el3(0);
}

void s32g_gic_setup(void)
{
#if IMAGE_BL31
	gicv3_driver_init(&s32g275_gic_data);
#endif
	gicv3_distif_init();
	gicv3_rdistif_init(plat_my_core_pos());
	gicv3_cpuif_enable(plat_my_core_pos());
}

void bl31_plat_arch_setup(void)
{
	s32g_smp_fixup();
	s32g_el3_mmu_fixup();

	/* kick secondary cores out of reset (but will leave them in wfi) */
	s32g_kick_secondary_ca53_cores();
}

static unsigned int plat_s32g275_mpidr_to_core_pos(unsigned long mpidr)
{
	return (unsigned int)plat_core_pos_by_mpidr(mpidr);
}

void bl31_platform_setup(void)
{
	s32g_gic_setup();
}

/* TODO: Last-minute modifications before exiting BL31:
 *  - restrict the S32G_PMEM_START..S32G_PMEM_END DRAM area only to
 *    secure privileged contexts;
 *  - lock XRDC until the next reset
 */
void bl31_plat_runtime_setup(void)
{
}

unsigned int plat_get_syscnt_freq2(void)
{
	return COUNTER_FREQUENCY;
}
