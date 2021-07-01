/*
 * Copyright 2019-2021 NXP
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
#include <plat/common/platform.h>

#include "drivers/generic_delay_timer.h"
#include "ocotp.h"
#include "platform_def.h"
#include "pmic/vr5510.h"
#include "s32g_pm.h"
#include "s32g_clocks.h"
#include "s32g_dt.h"
#include "s32g_linflexuart.h"
#include "s32g_lowlevel.h"
#include "s32g_mc_me.h"
#include "s32g_mc_rgm.h"
#include "s32g_ncore.h"
#include "s32g_pinctrl.h"
#include "s32g_xrdc.h"
#include "s32gen1-wkpu.h"
#include "s32g_bl_common.h"
#include "s32g_sramc.h"
#include "clk/clk.h"

#define MMU_ROUND_UP_TO_4K(x)	\
			(((x) & ~0xfff) == (x) ? (x) : ((x) & ~0xfff) + 0x1000)

IMPORT_SYM(uintptr_t, __RW_START__, BL31_RW_START);

static gicv3_redist_ctx_t rdisif_ctxs[PLATFORM_CORE_COUNT];
static gicv3_dist_ctx_t dist_ctx;

static const mmap_region_t s32g_mmap[] = {
	MAP_REGION_FLAT(S32G_SSRAM_BASE, S32G_SSRAM_LIMIT - S32G_SSRAM_BASE,
			 MT_MEMORY | MT_RW | MT_SECURE),
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
	MAP_REGION_FLAT(SRAMC0_BASE_ADDR, SRAMC_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(SSRAMC_BASE_ADDR, SRAMC_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION2(BL31SRAM_BASE, BL31SRAM_BASE,
		    MMU_ROUND_UP_TO_4K(BL31SRAM_SIZE),
		    MT_MEMORY | MT_RW, PAGE_SIZE),
	MAP_REGION2(S32G_BL33_IMAGE_BASE, S32G_BL33_IMAGE_BASE,
			MMU_ROUND_UP_TO_4K(S32G_BL33_IMAGE_SIZE),
			MT_MEMORY | MT_RW, PAGE_SIZE),
	MAP_REGION2(DTB_BASE, DTB_BASE, MMU_ROUND_UP_TO_4K(DTB_SIZE),
			MT_MEMORY | MT_RW, PAGE_SIZE),
	MAP_REGION_FLAT(S32G_PMEM_START, S32G_PMEM_LEN,
			MT_MEMORY | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32G_SCMI_SHARED_MEM, S32G_SCMI_SHARED_MEM_SIZE,
			MT_NON_CACHEABLE | MT_RW | MT_SECURE),
	{0},
};

static entry_point_info_t bl33_image_ep_info;
static entry_point_info_t bl32_image_ep_info;

static uintptr_t rdistif_base_addrs[PLATFORM_CORE_COUNT];

static const interrupt_prop_t interrupt_props[] = {
	INTR_PROP_DESC(S32G_SECONDARY_WAKE_SGI, GIC_HIGHEST_SEC_PRIORITY,
		       INTR_GROUP0, GIC_INTR_CFG_EDGE),
};

static unsigned int plat_s32g274a_mpidr_to_core_pos(unsigned long mpidr);

const gicv3_driver_data_t s32g274a_gic_data = {
	.gicd_base = PLAT_GICD_BASE,
	.gicr_base = PLAT_GICR_BASE,
	.rdistif_num = PLATFORM_CORE_COUNT,
	.rdistif_base_addrs = rdistif_base_addrs,
	.interrupt_props = interrupt_props,
	.interrupt_props_num = ARRAY_SIZE(interrupt_props),
	.mpidr_to_core_pos = plat_s32g274a_mpidr_to_core_pos,
};

volatile uint32_t s32g_core_release_var[PLATFORM_CORE_COUNT];

void update_core_state(uint32_t core, uint32_t state)
{
	s32g_core_release_var[core] = state;
	flush_dcache_range((uintptr_t)&s32g_core_release_var[core],
			   sizeof(s32g_core_release_var[core]));
}

bool is_last_core(void)
{
	size_t i, on = 0U;

	inv_dcache_range((uintptr_t)s32g_core_release_var,
			 sizeof(s32g_core_release_var));
	for (i = 0U; i < ARRAY_SIZE(s32g_core_release_var); i++)
		if (s32g_core_release_var[i])
			on++;

	return (on == 1);
}

bool is_cluster0_off(void)
{
	inv_dcache_range((uintptr_t)s32g_core_release_var,
			 sizeof(s32g_core_release_var));
	return !s32g_core_release_var[0] && !s32g_core_release_var[1];
}

bool is_cluster1_off(void)
{
	inv_dcache_range((uintptr_t)s32g_core_release_var,
			 sizeof(s32g_core_release_var));
	return !s32g_core_release_var[2] && !s32g_core_release_var[3];
}

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
	if (type == NON_SECURE)
		return &bl33_image_ep_info;
	else
		return &bl32_image_ep_info;
}

void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
		u_register_t arg2, u_register_t arg3)
{
	SET_PARAM_HEAD(&bl33_image_ep_info, PARAM_EP, VERSION_1, 0);
	bl33_image_ep_info.pc = BL33_ENTRYPOINT;
	bl33_image_ep_info.spsr = s32g_get_spsr_for_bl33_entry();
	SET_SECURITY_STATE(bl33_image_ep_info.h.attr, NON_SECURE);

#ifdef SPD_opteed
	SET_PARAM_HEAD(&bl32_image_ep_info, PARAM_EP, VERSION_2, 0);
	SET_SECURITY_STATE(bl32_image_ep_info.h.attr, SECURE);
	bl32_image_ep_info.pc = S32G_BL32_BASE;
	bl32_image_ep_info.spsr = 0;
	bl32_image_ep_info.args.arg0 = MODE_RW_64;
	bl32_image_ep_info.args.arg3 = BL33_DTB;
#endif
}

static void s32g_el3_mmu_fixup(void)
{
	const unsigned long code_start = BL_CODE_BASE;
	const unsigned long code_size = BL_CODE_END - BL_CODE_BASE;
	const unsigned long rw_start = BL31_RW_START;
	const unsigned long rw_size = BL_END - BL31_RW_START;
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
	_Static_assert(ARRAY_SIZE(s32g_mmap) + ARRAY_SIZE(regions) - 1 +
		       BL31SRAM_MAX_PAGES <= MAX_XLAT_TABLES,
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

void plat_gic_save(void)
{
	for (int i = 0; i < PLATFORM_CORE_COUNT; i++)
		gicv3_rdistif_save(i, &rdisif_ctxs[i]);

	gicv3_distif_save(&dist_ctx);
}

void plat_gic_restore(void)
{
	gicv3_distif_init_restore(&dist_ctx);

	for (int i = 0; i < PLATFORM_CORE_COUNT; i++)
		gicv3_rdistif_init_restore(i, &rdisif_ctxs[i]);
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

		i2c_driver = s32g_add_i2c_module(fdt, i2c_node);
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

static void dt_init_wkpu(void)
{
	void *fdt;
	int wkpu_node;
	int ret;

	if (dt_open_and_check() < 0) {
		INFO("ERROR fdt check\n");
		return;
	}

	if (fdt_get_address(&fdt) == 0) {
		INFO("ERROR fdt\n");
		return;
	}

	wkpu_node = fdt_node_offset_by_compatible(fdt, -1,
			"nxp,s32gen1-wkpu");
	if (wkpu_node == -1)
		return;


	ret = s32gen1_wkpu_init(fdt, wkpu_node);
	if (ret) {
		INFO("Failed to initialize WKPU\n");
		return;
	}
}

static void dt_init_ocotp(void)
{
	void *fdt;
	int ocotp_node;
	int ret;

	if (dt_open_and_check() < 0) {
		INFO("ERROR fdt check\n");
		return;
	}

	if (fdt_get_address(&fdt) == 0) {
		INFO("ERROR fdt\n");
		return;
	}

	ocotp_node = fdt_node_offset_by_compatible(fdt, -1,
			"fsl,s32g-ocotp");
	if (ocotp_node == -1)
		return;

	ret = s32gen1_ocotp_init(fdt, ocotp_node);
	if (ret) {
		INFO("Failed to initialize WKPU\n");
		return;
	}
}

void bl31_plat_arch_setup(void)
{
	s32g_smp_fixup();
	s32g_el3_mmu_fixup();

#if (S32G_USE_LINFLEX_IN_BL31 == 1)
	console_s32g_register();
#endif
}

static unsigned int plat_s32g274a_mpidr_to_core_pos(unsigned long mpidr)
{
	return (unsigned int)plat_core_pos_by_mpidr(mpidr);
}

static int check_clock_node(const void *fdt, int nodeoffset)
{
	const void *prop;
	int len;

	prop = fdt_getprop(fdt, nodeoffset, "assigned-clocks", &len);
	if (!prop)
		return len;

	return 0;

}

static int next_node_with_clocks(const void *fdt, int startoffset)
{
	int offset, err;

	for (offset = fdt_next_node(fdt, startoffset, NULL);
	     offset >= 0;
	     offset = fdt_next_node(fdt, offset, NULL)) {
		err = check_clock_node(fdt, offset);
		if ((err < 0) && (err != -FDT_ERR_NOTFOUND))
			return err;
		else if (err == 0)
			return offset;
	}

	return offset; /* error from fdt_next_node() */
}

void clk_tree_init(void)
{
	void *fdt;
	int clk_node;

	if (dt_open_and_check() < 0) {
		INFO("ERROR fdt check\n");
		return;
	}

	if (fdt_get_address(&fdt) == 0) {
		INFO("ERROR fdt\n");
		return;
	}

	clk_node = -1;
	while (true) {
		clk_node = next_node_with_clocks(fdt, clk_node);
		if (clk_node == -1)
			break;
	}
}

void bl31_platform_setup(void)
{
	int ret;

	generic_delay_timer_init();

	dt_init_pmic();
	dt_init_wkpu();
	dt_init_ocotp();

	ret = pmic_setup();
	if (ret)
		ERROR("Failed to disable VR5510 watchdog\n");

	update_core_state(plat_my_core_pos(), 1);
	s32g_gic_setup();

	dt_clk_init();
}

/* TODO: Last-minute modifications before exiting BL31:
 *  - restrict the S32G_PMEM_START..S32G_PMEM_END DRAM area only to
 *    secure privileged contexts;
 *  - lock XRDC until the next reset
 */
void bl31_plat_runtime_setup(void)
{
}
