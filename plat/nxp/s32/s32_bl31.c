/*
 * Copyright 2021-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch_helpers.h>
#include <assert.h>
#include <common/bl_common.h>
#include <clk/s32gen1_scmi_clk.h>
#include <drivers/arm/gicv3.h>
#include <libfdt.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <plat/common/platform.h>

#include "platform_def.h"
#include "s32_bl_common.h"
#include "s32_clocks.h"
#include "s32_dt.h"
#include "s32_linflexuart.h"
#include "s32_lowlevel.h"
#include "s32_mc_me.h"
#include "s32_mc_rgm.h"
#include "s32_ncore.h"
#include "s32_sramc.h"
#include "s32_interrupt_mgmt.h"
#include "s32_scp_scmi.h"

#define MMU_ROUND_UP_TO_4K(x)	\
	(((x) & ~0xfffU) == (x) ? (x) : ((x) & ~0xfffU) + 0x1000U)

/* Secondaries wake sgi + SCP IRQ + HSE IRQs */
#define MAX_INTR_PROPS	(2 + HSE_MU_INST)

IMPORT_SYM(uintptr_t, __RW_START__, BL31_RW_START);

static gicv3_redist_ctx_t rdisif_ctxs[PLATFORM_CORE_COUNT];
static gicv3_dist_ctx_t dist_ctx;

static const mmap_region_t s32_mmap[] = {
	MAP_REGION_FLAT(S32_UART_BASE, S32_UART_SIZE,
			MT_DEVICE | MT_RW | MT_NS),
	MAP_REGION_FLAT(S32GEN1_GIC_BASE, S32GEN1_GIC_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(S32_MC_ME_BASE_ADDR, S32_MC_ME_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(MC_CGM0_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(MC_CGM0_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(MC_CGM1_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(MC_CGM1_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(MC_CGM2_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(MC_CGM2_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(MC_CGM5_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(MC_CGM5_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(ARM_PLL_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(ARM_PLL_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(PERIPH_PLL_BASE_ADDR,
			MMU_ROUND_UP_TO_PAGE(PERIPH_PLL_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(ACCEL_PLL_BASE_ADDR,
			MMU_ROUND_UP_TO_PAGE(ACCEL_PLL_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(DRAM_PLL_BASE_ADDR,
			MMU_ROUND_UP_TO_PAGE(DRAM_PLL_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(S32_FXOSC_BASE_ADDR,
			MMU_ROUND_UP_TO_PAGE(S32_FXOSC_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(ARM_DFS_BASE_ADDR,
			MMU_ROUND_UP_TO_PAGE(ARM_DFS_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(PERIPH_DFS_BASE_ADDR,
			MMU_ROUND_UP_TO_PAGE(PERIPH_DFS_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(GPR_BASE_PAGE_ADDR, MMU_ROUND_UP_TO_PAGE(GPR_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(S32_MC_RGM_BASE_ADDR, S32_MC_RGM_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(RDC_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(RDC_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(I2C4_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(I2C4_SIZE),
			MT_DEVICE | MT_RW),
	/* When we execute at System Monitor on behalf of EL2/EL1, we might
	 * have to reconfigure Ncore
	 */
	MAP_REGION_FLAT(NCORE_BASE_ADDR, S32_NCORE_SIZE,
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(SRAMC0_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(SRAMC_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(SRAMC1_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(SRAMC_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(SIUL2_0_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(SIUL2_0_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(SIUL2_1_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(SIUL2_1_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION2(BL33_DTB, BL33_DTB,
			MMU_ROUND_UP_TO_4K(S32_BL33_IMAGE_SIZE),
			MT_MEMORY | MT_RW, PAGE_SIZE),
	MAP_REGION_FLAT(S32_PMEM_START, S32_PMEM_LEN,
			MT_MEMORY | MT_RW | MT_SECURE),
	MAP_REGION_FLAT(S32_OSPM_SCMI_MEM,
			MMU_ROUND_UP_TO_PAGE(S32_OSPM_SCMI_MEM_SIZE),
			MT_NON_CACHEABLE | MT_RW | MT_SECURE),
	/* SCP entries */
	MAP_REGION_FLAT(MSCM_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(MSCM_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(S32_SCP_SCMI_MEM,
			MMU_ROUND_UP_TO_PAGE(S32_SCP_SCMI_SIZE),
			MT_NON_CACHEABLE | MT_RW | MT_SECURE),
#if defined(STM6_BASE_ADDR)
	MAP_REGION_FLAT(STM6_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(STM6_SIZE),
			MT_DEVICE | MT_RW),
#endif
#if defined(MC_CGM6_BASE_ADDR)
	MAP_REGION_FLAT(MC_CGM6_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(MC_CGM6_SIZE),
			MT_DEVICE | MT_RW),
#endif
#if defined(SRAMC2_BASE_ADDR)
	MAP_REGION_FLAT(SRAMC2_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(SRAMC_SIZE),
			MT_DEVICE | MT_RW),
#endif
#if defined(SRAMC3_BASE_ADDR)
	MAP_REGION_FLAT(SRAMC3_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(SRAMC_SIZE),
			MT_DEVICE | MT_RW),
#endif
#if defined(SSRAMC_BASE_ADDR)
	MAP_REGION_FLAT(SSRAMC_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(SRAMC_SIZE),
			MT_DEVICE | MT_RW),
#endif
#if defined(WKPU_BASE_ADDR)
	MAP_REGION_FLAT(WKPU_BASE_ADDR, MMU_ROUND_UP_TO_PAGE(WKPU_SIZE),
			MT_DEVICE | MT_RW),
#endif
#if defined(S32G_RTC_BASE)
	MAP_REGION_FLAT(S32G_RTC_BASE,
			MMU_ROUND_UP_TO_PAGE(S32G_RTC_SIZE),
			MT_DEVICE | MT_RW),
#endif
#if defined(S32G_SSRAM_BASE)
	MAP_REGION_FLAT(S32G_SSRAM_BASE, S32G_SSRAM_LIMIT - S32G_SSRAM_BASE,
			 MT_MEMORY | MT_RW | MT_SECURE),
#endif
#if defined(BL31SRAM_BASE)
	MAP_REGION2(BL31SRAM_BASE, BL31SRAM_BASE,
		    MMU_ROUND_UP_TO_4K(BL31SRAM_SIZE),
		    MT_MEMORY | MT_RW, PAGE_SIZE),
#endif
	{0},
};

static entry_point_info_t bl33_image_ep_info;
static entry_point_info_t bl32_image_ep_info;

static uintptr_t rdistif_base_addrs[PLATFORM_CORE_COUNT];

static interrupt_prop_t interrupt_props[MAX_INTR_PROPS];

static unsigned int plat_s32_mpidr_to_core_pos(unsigned long mpidr);

static gicv3_driver_data_t s32_gic_data = {
	.gicd_base = PLAT_GICD_BASE,
	.gicr_base = PLAT_GICR_BASE,
	.rdistif_num = PLATFORM_CORE_COUNT,
	.rdistif_base_addrs = rdistif_base_addrs,
	.interrupt_props = interrupt_props,
	.interrupt_props_num = ARRAY_SIZE(interrupt_props),
	.mpidr_to_core_pos = plat_s32_mpidr_to_core_pos,
};

volatile uint32_t s32_core_release_var[PLATFORM_CORE_COUNT];
DEFINE_BAKERY_LOCK(s32_core_state_lock);

void update_core_state(uint32_t core, uint32_t mask, uint32_t flag)
{
	bakery_lock_get(&s32_core_state_lock);
	inv_dcache_range((uintptr_t)s32_core_release_var,
			 sizeof(s32_core_release_var));
	s32_core_release_var[core] &= ~mask;
	s32_core_release_var[core] |= flag;
	flush_dcache_range((uintptr_t)&s32_core_release_var[core],
			   sizeof(s32_core_release_var[core]));
	bakery_lock_release(&s32_core_state_lock);
}

uint32_t get_core_state(uint32_t core, uint32_t mask)
{
	uint32_t status;

	bakery_lock_get(&s32_core_state_lock);
	inv_dcache_range((uintptr_t)s32_core_release_var,
			 sizeof(s32_core_release_var));
	status = s32_core_release_var[core] & mask;
	bakery_lock_release(&s32_core_state_lock);

	return status;
}

/**
 * The caller must take 's32_core_state_lock'
 * and invalidate the caches on 's32_core_release_var'.
 */
static bool is_cpu_on(uint32_t core)
{
	return !!(s32_core_release_var[core] & CPU_ON);
}

bool is_core_enabled(uint32_t core)
{
	bool status;

	bakery_lock_get(&s32_core_state_lock);
	inv_dcache_range((uintptr_t)s32_core_release_var,
			 sizeof(s32_core_release_var));
	status = is_cpu_on(core);
	bakery_lock_release(&s32_core_state_lock);

	return status;
}

bool is_last_core(void)
{
	size_t i, on = 0U;

	bakery_lock_get(&s32_core_state_lock);
	inv_dcache_range((uintptr_t)s32_core_release_var,
			 sizeof(s32_core_release_var));
	for (i = 0U; i < ARRAY_SIZE(s32_core_release_var); i++) {
		if (is_cpu_on(i))
			on++;
	}
	bakery_lock_release(&s32_core_state_lock);

	return (on == 1);
}

bool is_cluster0_off(void)
{
	size_t i;
	bool off_status = true;

	bakery_lock_get(&s32_core_state_lock);
	inv_dcache_range((uintptr_t)s32_core_release_var,
			 sizeof(s32_core_release_var));
	for (i = 0U; i < PLATFORM_CORE_COUNT / 2; i++) {
		if (is_cpu_on(i))
			off_status = false;

		if (!off_status)
			break;
	}
	bakery_lock_release(&s32_core_state_lock);

	return off_status;
}

bool is_cluster1_off(void)
{
	size_t i;
	bool off_status = true;

	bakery_lock_get(&s32_core_state_lock);
	inv_dcache_range((uintptr_t)s32_core_release_var,
			 sizeof(s32_core_release_var));
	for (i = PLATFORM_CORE_COUNT / 2; i < PLATFORM_CORE_COUNT; i++) {

		if (is_cpu_on(i)) {
			off_status = false;
			break;
		}
	}
	bakery_lock_release(&s32_core_state_lock);

	return off_status;
}

static uint32_t s32_get_spsr_for_bl33_entry(void)
{
	uint32_t spsr;
	unsigned long el_status, mode;

	/* figure out what mode we enter the non-secure world */
	el_status = read_id_aa64pfr0_el1() >> ID_AA64PFR0_EL2_SHIFT;
	el_status &= ID_AA64PFR0_ELX_MASK;
#if (S32_HAS_HV == 0)
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
	bl33_image_ep_info.spsr = s32_get_spsr_for_bl33_entry();
	SET_SECURITY_STATE(bl33_image_ep_info.h.attr, NON_SECURE);

#ifdef SPD_opteed
	SET_PARAM_HEAD(&bl32_image_ep_info, PARAM_EP, VERSION_2, 0);
	SET_SECURITY_STATE(bl32_image_ep_info.h.attr, SECURE);
	bl32_image_ep_info.pc = S32_BL32_BASE;
	bl32_image_ep_info.spsr = 0;
	bl32_image_ep_info.args.arg0 = MODE_RW_64;
	bl32_image_ep_info.args.arg3 = BL33_DTB;
#endif
}

static uintptr_t get_dtb_base_page(void)
{
	return get_bl2_dtb_base() & ~PAGE_MASK;
}

static void s32_el3_mmu_fixup(void)
{
	const unsigned long code_start = BL_CODE_BASE;
	const unsigned long rw_start = BL31_RW_START;
	unsigned long code_size;
	unsigned long rw_size;

	if (BL_END < BL31_RW_START)
		panic();

	if (BL_CODE_END < BL_CODE_BASE)
		panic();

	code_size = BL_CODE_END - BL_CODE_BASE;
	rw_size = BL_END - BL31_RW_START;

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
		{
			.base_pa = get_dtb_base_page(),
			.base_va = get_dtb_base_page(),
			.size = MMU_ROUND_UP_TO_4K(dtb_size),
			.attr = MT_RO | MT_MEMORY | MT_SECURE,
		},
	};
	int i;

	/* The calls to mmap_add_region() consume mmap regions,
	 * so they must be counted in the static asserts
	 */
	_Static_assert(ARRAY_SIZE(s32_mmap) + ARRAY_SIZE(regions) - 1 <=
		MAX_MMAP_REGIONS,
		"Fewer MAX_MMAP_REGIONS than in s32_mmap will likely result in a MMU exception at runtime");

	/* MMU initialization; while technically not necessary on cold boot,
	 * it is required for warm boot path processing
	 */
	for (i = 0; i < ARRAY_SIZE(regions); i++)
		mmap_add_region(regions[i].base_pa, regions[i].base_va,
				regions[i].size, regions[i].attr);

	mmap_add(s32_mmap);

	init_xlat_tables();
	enable_mmu_el3(0);
}

static interrupt_prop_t *register_and_check_irq(interrupt_prop_t *itr,
						const interrupt_prop_t *end,
						interrupt_prop_t *irq_prop)
{
	if (itr == end || itr == NULL || !irq_prop)
		return NULL;

	if (check_uptr_overflow((uintptr_t)itr, 1))
		return NULL;

	*itr = *irq_prop;

	return ++itr;
}

static interrupt_prop_t *register_cpu_wake_irq(interrupt_prop_t *itr,
					       const interrupt_prop_t *end)
{
	interrupt_prop_t irq_prop = INTR_PROP_DESC(S32_SECONDARY_WAKE_SGI,
						   GIC_HIGHEST_SEC_PRIORITY,
						   INTR_GROUP0,
						   GIC_INTR_CFG_EDGE);

	itr = register_and_check_irq(itr, end, &irq_prop);
	if (!itr)
		ERROR("Failed to register CPU wake IRQ\n");

	return itr;
}

static interrupt_prop_t *register_scp_notif_irq(interrupt_prop_t *itr,
						const interrupt_prop_t *end)
{
	interrupt_prop_t irq_prop;
	int rx_irq_num = scp_get_rx_plat_irq();

	if (rx_irq_num < 0) {
		ERROR("Invalid SCP notification IRQ: %d\n", rx_irq_num);
		return NULL;
	}

	irq_prop = (interrupt_prop_t)INTR_PROP_DESC(rx_irq_num,
						   GIC_HIGHEST_SEC_PRIORITY,
						   INTR_GROUP0,
						   GIC_INTR_CFG_EDGE);

	itr = register_and_check_irq(itr, end, &irq_prop);
	if (!itr)
		ERROR("Failed to register SCP notification IRQ\n");

	return itr;
}

static interrupt_prop_t *register_hse_irqs(interrupt_prop_t *itr,
					   const interrupt_prop_t *end)
{
	int offs = -1, ret = 0, rx_irq_off, rx_irq_num;
	interrupt_prop_t irq_prop;
	void *fdt = (void *)BL33_DTB;

	ret = fdt_check_header(fdt);
	if (ret < 0) {
		INFO("ERROR fdt check\n");
		return itr;
	}

	while (true) {
		offs = fdt_node_offset_by_compatible(fdt, offs,
						     "nxp,s32cc-hse");

		if (offs == -FDT_ERR_NOTFOUND)
			break;

		if (fdt_get_status(offs) != DT_ENABLED)
			continue;

		rx_irq_off = fdt_stringlist_search(fdt, offs,
						   "interrupt-names",
						   "hse-rx");
		if (rx_irq_off < 0) {
			ret = rx_irq_off;
			break;
		}

		ret = fdt_get_irq_props_by_index(fdt, offs, 0, rx_irq_off, &rx_irq_num);
		if (ret < 0)
			break;

		if (rx_irq_num < 0) {
			ERROR("Invalid HSE IRQ %d\n", rx_irq_num);
			ret = -EINVAL;
			break;
		}

		irq_prop = (interrupt_prop_t)
		    INTR_PROP_DESC(rx_irq_num,
				   GIC_HIGHEST_SEC_PRIORITY,
				   INTR_GROUP1S,
				   GIC_INTR_CFG_EDGE);

		itr = register_and_check_irq(itr, end, &irq_prop);
		if (!itr) {
			ret = -EINVAL;
			ERROR("Failed to register HSE IRQ %d\n", rx_irq_num);
			break;
		}
	}

	/* Disable the node if something goes wrong */
	if (ret) {
		fdt_setprop_string(fdt, offs, "status", "disabled");
		flush_dcache_range((uintptr_t)fdt, fdt_totalsize(fdt));
	}

	return itr;
}

static void register_irqs(void)
{
	interrupt_prop_t *itr, *end;
	bool has_hse = false;

#if defined(HSE_SUPPORT) && defined(SPD_opteed)
	has_hse = true;
#endif

	itr = &interrupt_props[0];
	end = &interrupt_props[ARRAY_SIZE(interrupt_props)];

	itr = register_cpu_wake_irq(itr, end);

	if (is_scp_used())
		itr = register_scp_notif_irq(itr, end);

	if (has_hse)
		itr = register_hse_irqs(itr, end);

	if (!itr)
		return;

	s32_gic_data.interrupt_props_num = itr - &interrupt_props[0];
}

void s32_gic_setup(void)
{
	unsigned int pos = plat_my_core_pos();

	gicv3_driver_init(&s32_gic_data);
	gicv3_distif_init();
	gicv3_rdistif_init(pos);
	gicv3_cpuif_enable(pos);
	update_core_state(pos, CPUIF_EN, CPUIF_EN);
}

void plat_gic_save(void)
{
	unsigned int i;

	for (i = 0; i < PLATFORM_CORE_COUNT; i++) {
		if (get_core_state(i, CPUIF_EN)) {
			gicv3_cpuif_disable(i);
			update_core_state(i, CPUIF_EN, 0);
		}
	}

	for (i = 0; i < PLATFORM_CORE_COUNT; i++) {
		gicv3_rdistif_save(i, &rdisif_ctxs[i]);
	}

	gicv3_distif_save(&dist_ctx);
}

void plat_gic_restore(void)
{
	gicv3_distif_init_restore(&dist_ctx);

	for (int i = 0; i < PLATFORM_CORE_COUNT; i++)
		gicv3_rdistif_init_restore(i, &rdisif_ctxs[i]);
}

void bl31_plat_arch_setup(void)
{
	s32_smp_fixup();
	s32_el3_mmu_fixup();

#if (S32_USE_LINFLEX_IN_BL31 == 1)
	console_s32_register();
#endif

	if (is_scp_used())
		scp_scmi_init(true);

	register_irqs();
}

static unsigned int plat_s32_mpidr_to_core_pos(unsigned long mpidr)
{
	return (unsigned int)plat_core_pos_by_mpidr(mpidr);
}

void bl31_plat_runtime_setup(void)
{
	int rx_irq_num = scp_get_rx_plat_irq();

	if (is_scp_used()) {
		s32cc_el3_interrupt_config();

		if (rx_irq_num < 0) {
			ERROR("Invalid SCP notification IRQ: %d\n", rx_irq_num);
			return;
		}

		/* Route the irq to any available core */
		plat_ic_set_spi_routing(rx_irq_num,
					INTR_ROUTING_MODE_ANY,
					read_mpidr());
	} else {
		/* Mark A53 clock as enabled */
		update_a53_clk_state(true);
	}
}

