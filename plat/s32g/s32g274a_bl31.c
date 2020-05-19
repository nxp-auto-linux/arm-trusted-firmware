/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <common/bl_common.h>
#include <drivers/arm/gicv3.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <libfdt.h>
#include <psci.h>

#include "drivers/generic_delay_timer.h"
#include "i2c/s32g274a_i2c.h"
#include "platform_def.h"
#include "pmic/vr5510.h"
#include "s32g274a_pm.h"
#include "s32g_clocks.h"
#include "s32g_dt.h"
#include "s32g_linflexuart.h"
#include "s32g_lowlevel.h"
#include "s32g_mc_me.h"
#include "s32g_mc_rgm.h"
#include "s32g_ncore.h"
#include "s32g_pinctrl.h"
#include "s32g_xrdc.h"

#define S32G_MAX_I2C_MODULES 5

#define MMU_ROUND_UP_TO_4K(x)	\
			(((x) & ~0xfff) == (x) ? (x) : ((x) & ~0xfff) + 0x1000)

struct s32g_i2c_driver {
	struct s32g_i2c_bus bus;
	int fdt_node;
};

IMPORT_SYM(uintptr_t, __RW_START__, BL31_RW_START);
IMPORT_SYM(uintptr_t, __RW_END__, BL31_RW_END);

static const mmap_region_t s32g_mmap[] = {
	MAP_REGION_FLAT(S32G_UART_BASE, S32G_UART_SIZE,
			MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(S32G274A_GIC_BASE, S32G274A_GIC_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(S32G_XRDC_BASE, S32G_XRDC_SIZE,
			MT_DEVICE | MT_RW | MT_SECURE),
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
	/* When we execute at System Monitor on behalf of EL2/EL1, we might
	 * have to reconfigure Ncore
	 */
	MAP_REGION_FLAT(NCORE_BASE_ADDR, S32G_NCORE_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(BL33_ENTRYPOINT,
			MMU_ROUND_UP_TO_4K(S32G_BL33_IMAGE_SIZE),
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
	INTR_PROP_DESC(S32G_RTC_INT, GIC_HIGHEST_SEC_PRIORITY,
		       INTR_GROUP0, GIC_INTR_CFG_EDGE),
};

static unsigned int plat_s32g274a_mpidr_to_core_pos(unsigned long mpidr);
/* Declare it here to avoid including plat/common/platform.h */
unsigned int plat_my_core_pos(void);

const gicv3_driver_data_t s32g274a_gic_data = {
	.gicd_base = PLAT_GICD_BASE,
	.gicr_base = PLAT_GICR_BASE,
	.rdistif_num = PLATFORM_CORE_COUNT,
	.rdistif_base_addrs = rdistif_base_addrs,
	.interrupt_props = interrupt_props,
	.interrupt_props_num = ARRAY_SIZE(interrupt_props),
	.mpidr_to_core_pos = plat_s32g274a_mpidr_to_core_pos,
};


static uint32_t s32g_get_spsr_for_bl33_entry(void)
{
	uint32_t spsr;
	unsigned long el_status, mode;

	/* figure out what mode we enter the non-secure world */
	el_status = read_id_aa64pfr0_el1() >> ID_AA64PFR0_EL2_SHIFT;
	el_status &= ID_AA64PFR0_ELX_MASK;
#if (S32G_HAS_HV == 0)
	mode = MODE_EL1;
#else
	mode = (el_status) ? MODE_EL2 : MODE_EL1;
#endif
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
	SET_PARAM_HEAD(&bl33_image_ep_info, PARAM_EP, VERSION_1, 0);
	bl33_image_ep_info.pc = BL33_ENTRYPOINT;
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
	gicv3_driver_init(&s32g274a_gic_data);
	gicv3_distif_init();
	gicv3_rdistif_init(plat_my_core_pos());
	gicv3_cpuif_enable(plat_my_core_pos());
}

static struct s32g_i2c_driver *init_i2c_module(void *fdt, int fdt_node)
{
	static struct s32g_i2c_driver i2c_drivers[S32G_MAX_I2C_MODULES];
	static size_t fill_level;
	struct s32g_i2c_driver *driver;
	struct dt_node_info i2c_info;
	size_t i;
	int ret;

	ret = fdt_node_check_compatible(fdt, fdt_node, "fsl,vf610-i2c");
	if (ret)
		return NULL;

	for (i = 0; i < fill_level; i++) {
		if (i2c_drivers[i].fdt_node == fdt_node)
			return &i2c_drivers[i];
	}

	if (fill_level >= ARRAY_SIZE(i2c_drivers)) {
		INFO("Discovered too many instances of I2C\n");
		return NULL;
	}

	driver = &i2c_drivers[fill_level];

	dt_fill_device_info(&i2c_info, fdt_node);

	if (i2c_info.base == 0U) {
		INFO("ERROR i2c base\n");
		return NULL;
	}

	driver->fdt_node = fdt_node;
	s32g_i2c_get_setup_from_fdt(fdt, fdt_node, &driver->bus);
	s32g_i2c_init(&driver->bus);

	fill_level++;
	return driver;
}

static void dt_init_pmic(void)
{
	void *fdt;
	int pmic_node;
	int i2c_node;
	struct s32g_i2c_driver *i2c_driver;
	int ret;

	if (dt_open_and_check() < 0) {
		INFO("ERROR fdt check\n");
		return;
	}

	if (fdt_get_address(&fdt) == 0) {
		INFO("ERROR fdt\n");
		return;
	}

	pmic_node = -1;
	while (true) {
		pmic_node = fdt_node_offset_by_compatible(fdt, pmic_node,
				"fsl,vr5510");
		if (pmic_node == -1)
			break;

		i2c_node = fdt_parent_offset(fdt, pmic_node);
		if (i2c_node == -1) {
			INFO("Failed to get parent of PMIC node\n");
			return;
		}

		i2c_driver = init_i2c_module(fdt, i2c_node);
		if (i2c_driver == NULL) {
			INFO("PMIC isn't subnode of an I2C node\n");
			return;
		}

		ret = vr5510_register_instance(fdt, pmic_node,
					       &i2c_driver->bus);
		if (ret) {
			INFO("Failed to register VR5510 instance\n");
			return;
		}
	}
}

void bl31_plat_arch_setup(void)
{
	static struct console_s32g console;

	s32g_smp_fixup();
	s32g_el3_mmu_fixup();

	console_s32g_register(S32G_UART_BASE, S32G_UART_CLOCK_HZ,
			S32G_UART_BAUDRATE, &console);

	/* kick secondary cores out of reset (but will leave them in wfi) */
	s32g_kick_secondary_ca53_cores();
}

static unsigned int plat_s32g274a_mpidr_to_core_pos(unsigned long mpidr)
{
	return (unsigned int)plat_core_pos_by_mpidr(mpidr);
}

void bl31_platform_setup(void)
{
	s32g_gic_setup();

	generic_delay_timer_init();

	dt_init_pmic();
}

/* TODO: Last-minute modifications before exiting BL31:
 *  - restrict the S32G_PMEM_START..S32G_PMEM_END DRAM area only to
 *    secure privileged contexts;
 *  - lock XRDC until the next reset
 */
void bl31_plat_runtime_setup(void)
{
}
