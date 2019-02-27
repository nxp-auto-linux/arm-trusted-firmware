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

#include "platform_def.h"
#include "s32g_psci.h"

IMPORT_SYM(uintptr_t, __RO_START__, BL31_RO_START);
IMPORT_SYM(uintptr_t, __RO_END__, BL31_RO_END);
IMPORT_SYM(uintptr_t, __RW_START__, BL31_RW_START);
IMPORT_SYM(uintptr_t, __RW_END__, BL31_RW_END);

static const mmap_region_t s32g_mmap[] = {
	MAP_REGION_FLAT(S32G_UART_BASE, S32G_UART_SIZE,
			MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(S32G275_GIC_BASE, S32G275_GIC_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(S32G_XRDC_BASE, S32G_XRDC_SIZE,
			MT_DEVICE | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32G_PMEM_START, S32G_PMEM_LEN,
			MT_MEMORY | MT_RW | MT_SECURE),
	{0},
};

static entry_point_info_t bl33_image_ep_info;

/* Declare it here to avoid including plat/common/platform.h */
unsigned int plat_my_core_pos(void);


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
	static struct console_s32g console;

#if RESET_TO_BL31
	assert((void *)arg0 == NULL); /* from bl2 */
	assert((void *)arg1 == NULL); /* plat params from bl2 */
#endif

	SET_PARAM_HEAD(&bl33_image_ep_info, PARAM_EP, VERSION_1, 0);
	bl33_image_ep_info.pc = S32G_BL33_IMAGE_BASE;
	bl33_image_ep_info.spsr = s32g_get_spsr_for_bl33_entry();
	SET_SECURITY_STATE(bl33_image_ep_info.h.attr, NON_SECURE);

	console_s32g_register(S32G_UART_BASE, S32G_UART_CLOCK_HZ,
			S32G_UART_BAUDRATE, &console);
}

static void s32g_el3_mmu_fixup(void)
{
	unsigned long rw_start = BL31_RW_START;
	unsigned long rw_size = BL31_RW_END - BL31_RW_START;
	unsigned long code_start = BL_CODE_BASE;
	unsigned long code_size = BL_CODE_END - BL_CODE_BASE;

	/* MMU initialization; while technically not necessary on cold boot,
	 * it is required for warm boot path processing
	 */
	mmap_add_region(code_start, code_start, code_size,
		MT_CODE | MT_SECURE);
	mmap_add_region(rw_start, rw_start, rw_size,
		MT_RW | MT_MEMORY | MT_SECURE);
	mmap_add(s32g_mmap);

	init_xlat_tables();
	enable_mmu_el3(0);
}

void bl31_plat_arch_setup(void)
{

	s32g_smp_fixup();
	s32g_el3_mmu_fixup();
}

static unsigned int plat_s32g275_mpidr_to_core_pos(unsigned long mpidr)
{
	return (unsigned int)plat_core_pos_by_mpidr(mpidr);
}

static uintptr_t rdistif_base_addrs[PLATFORM_CORE_COUNT];

static const interrupt_prop_t interrupt_props[] = {
	INTR_PROP_DESC(29, GIC_HIGHEST_SEC_PRIORITY,
		       INTR_GROUP0, GIC_INTR_CFG_EDGE),
	INTR_PROP_DESC(153, GIC_HIGHEST_NS_PRIORITY,
		       INTR_GROUP1S, GIC_INTR_CFG_LEVEL),
};

const gicv3_driver_data_t s32g275_gic_data = {
	.gicd_base = PLAT_GICD_BASE,
	.gicr_base = PLAT_GICR_BASE,
	.rdistif_num = PLATFORM_CORE_COUNT,
	.rdistif_base_addrs = rdistif_base_addrs,
	.interrupt_props = interrupt_props,
	.interrupt_props_num = ARRAY_SIZE(interrupt_props),
	.mpidr_to_core_pos = plat_s32g275_mpidr_to_core_pos,
};

void bl31_platform_setup(void)
{
#if IMAGE_BL31
	gicv3_driver_init(&s32g275_gic_data);
#endif

	gicv3_distif_init();
	gicv3_rdistif_init(plat_my_core_pos());
	gicv3_cpuif_enable(plat_my_core_pos());
}

/* Last-minute modifications before exiting BL31:
 *  - install the PSCI handlers at S32G_PMEM_START;
 *  - restrict the S32G_PMEM_START..S32G_PMEM_END DRAM area only to
 *    secure privileged contexts;
 *  - lock XRDC until the next reset
 */
void bl31_plat_runtime_setup(void)
{
	s32g_psci_move_to_pram();
	/* If we enable XRDC, the functional simulator will screech to a halt;
	 * until a fix is provided, we'll just skip it
	 */
	INFO("Setting up XRDC...\n");
	if (xrdc_enable((void *)S32G_XRDC_BASE))
		ERROR("%s(): Error initializing XRDC!\n", __func__);
}

unsigned int plat_get_syscnt_freq2(void)
{
	return COUNTER_FREQUENCY;
}
