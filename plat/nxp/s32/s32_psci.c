/*
 * Copyright 2019-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "platform_def.h"
#include "s32_lowlevel.h"
#include "s32_ncore.h"

#if defined(PLAT_s32g2) || defined(PLAT_s32g3)
#include "bl31_sram.h"
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <plat/nxp/s32g/bl31_ssram/ssram_mailbox.h>
#include "s32_sramc.h"
#include "s32gen1-wkpu.h"
#include "s32g_bl_common.h"
#include "s32g_clocks.h"
#include "s32g_mc_me.h"
#include "s32g_resume.h"
#include "s32g_vr5510.h"
#else
#include "s32_bl_common.h"
#include "s32_mc_me.h"
#endif

#include <arch_helpers.h>
#include <assert.h>
#include <common/debug.h>	/* printing macros such as INFO() */
#include <drivers/arm/gicv3.h>
#include <plat/common/platform.h>

/* See firmware-design, psci-lib-integration-guide for details */
/* Used by plat_secondary_cold_boot_setup */
uintptr_t s32_warmboot_entry;

static const unsigned char s32_power_domain_tree_desc[] = {
	PLATFORM_SYSTEM_COUNT,
	PLATFORM_CLUSTER_COUNT,
	PLATFORM_CORE_COUNT / 2,
	PLATFORM_CORE_COUNT / 2
};

static bool is_core_in_secondary_cluster(int pos)
{
	return (pos >= PLATFORM_CORE_COUNT / 2);
}

/** Executed by the primary core as part of the PSCI_CPU_ON call,
 *  e.g. during Linux kernel boot.
 */
static int s32_pwr_domain_on(u_register_t mpidr)
{
	int pos;
	uintptr_t core_start_addr = (uintptr_t)&plat_secondary_cold_boot_setup;

	pos = plat_core_pos_by_mpidr(mpidr);
	dsbsy();

	if (s32_core_in_reset(pos))
		s32_kick_secondary_ca53_core(pos, core_start_addr);

	/* Do some chores on behalf of the secondary core. ICC setup must be
	 * done by the secondaries, because the interface is not memory-mapped.
	 */
	gicv3_rdistif_init(pos);
	/* GICR_IGROUPR0, GICR_IGRPMOD0 */
	gicv3_set_interrupt_type(S32_SECONDARY_WAKE_SGI, pos, INTR_GROUP0);
	/* GICR_ISENABLER0 */
	assert(plat_ic_is_sgi(S32_SECONDARY_WAKE_SGI));
	gicv3_enable_interrupt(S32_SECONDARY_WAKE_SGI, pos);

	/* Kick the secondary core out of wfi */
	NOTICE("S32 TF-A: %s: booting up core %d\n", __func__, pos);
	update_core_state(pos, 1);
	plat_ic_raise_el3_sgi(S32_SECONDARY_WAKE_SGI, mpidr);

	if (is_core_in_secondary_cluster(pos) &&
	    !ncore_is_caiu_online(A53_CLUSTER1_CAIU))
		ncore_caiu_online(A53_CLUSTER1_CAIU);

	if (!is_core_in_secondary_cluster(pos) &&
	    !ncore_is_caiu_online(A53_CLUSTER0_CAIU))
		ncore_caiu_online(A53_CLUSTER0_CAIU);

	return PSCI_E_SUCCESS;
}

/** Executed by the woken (secondary) core after it exits the wfi holding pen
 *  during cold boot.
 */
static void s32_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	int pos;
	unsigned int intid;

	NOTICE("S32 TF-A: %s: cpu %d running\n", __func__, plat_my_core_pos());

	/* Clear pending interrupt */
	pos = plat_my_core_pos();
	while ((intid = gicv3_get_pending_interrupt_id()) <= MAX_SPI_ID) {
		gicv3_clear_interrupt_pending(intid, pos);

		if (intid == S32_SECONDARY_WAKE_SGI)
			break;

		WARN("%s(): Interrupt %d found pending instead of the expected %d\n",
		     __func__, intid, S32_SECONDARY_WAKE_SGI);
	}

	write_scr_el3(read_scr_el3() & ~SCR_IRQ_BIT);
}
#if defined(PLAT_s32g2) || defined(PLAT_s32g3)
#if S32G_EMU == 0
static void copy_bl31sram_image(void)
{
	uint32_t npages;
	int ret;

	/* Clear all BL31SRAM sections */
	ret = s32_sram_clear(BL31SRAM_BASE, BL31SRAM_LIMIT);
	if (ret)
		ERROR("Failed to initialize SRAM from BL31SRAM stage\n");

	npages = bl31sram_len / PAGE_SIZE;
	if (bl31sram_len % PAGE_SIZE)
		npages++;

	ret = xlat_change_mem_attributes(BL31SRAM_BASE,
					 npages * PAGE_SIZE,
					 MT_MEMORY | MT_RW | MT_EXECUTE_NEVER);
	if (ret)
		ERROR("Failed to change the attributes of BL31 SRAM memory\n");

	/* Copy bl31 sram stage */
	memcpy((void *)BL31SRAM_BASE, bl31sram, bl31sram_len);
	ret = xlat_change_mem_attributes(BL31SRAM_BASE,
			npages * PAGE_SIZE,
			MT_CODE | MT_SECURE);
	if (ret)
		ERROR("Failed to change the attributes of BL31 SRAM memory\n");
}
#endif

static void bl31sram_entry(void)
{
	bl31_sram_entry_t entry;

	entry = (void *)BL31SRAM_BASE;
	entry();
}

static void set_warm_entry(void)
{
	uintptr_t warm_entry, short_boot;

	warm_entry = BL31SSRAM_MAILBOX + offsetof(struct s32g_ssram_mailbox,
						  bl31_warm_entrypoint);
	short_boot = BL31SSRAM_MAILBOX + offsetof(struct s32g_ssram_mailbox,
						  short_boot);
	mmio_write_64(warm_entry, (uintptr_t)s32g_resume_entrypoint);
	mmio_write_8(short_boot, (uint8_t)s32gen1_is_wkp_short_boot());
}

static void __dead2 platform_suspend(unsigned int current_cpu)
{
	size_t i;
	size_t ncores = PLATFORM_CORE_COUNT;

	for (i = 0; i < PLATFORM_CORE_COUNT; i++)
		gicv3_cpuif_disable(i);

	plat_gic_save();
	set_warm_entry();
	pmic_prepare_for_suspend();
	s32gen1_wkpu_enable_irqs();

	/* Shutting down cores */
	/* M7 cores */
	s32_turn_off_mcores();

	if (is_lockstep_enabled())
		ncores /= 2;

	/* A53 cores */
	for (i = 0; i < ncores; i++) {
		if (i != current_cpu)
			s32_turn_off_core(S32_MC_ME_CA53_PART, i);
	}

	/* PFE blocks */
	s32_disable_cofb_clk(S32G_MC_ME_PFE_PART, 0);
	/* Keep the DDR clock */
	s32_disable_cofb_clk(S32_MC_ME_USDHC_PART,
			      S32_MC_ME_PRTN_N_REQ(S32_MC_ME_DDR_0_REQ));

	/* Switching all MC_CGM muxes to FIRC */
	s32g_sw_clks2firc();

	/* Turn off DFS */
	s32g_disable_dfs(S32_PERIPH_DFS);
	s32g_disable_dfs(S32_CORE_DFS);

	/* Turn off PLL */
	s32g_disable_pll(S32_ACCEL_PLL, 2);
	s32g_disable_pll(S32_PERIPH_PLL, 8);
	s32g_disable_pll(S32_CORE_PLL, 2);

	bl31sram_entry();
	plat_panic_handler();
}

static void __dead2 s32g_pwr_domain_pwr_down_wfi(
					const psci_power_state_t *target_state)
{
	unsigned int pos = plat_my_core_pos();

	NOTICE("S32G TF-A: %s: cpu = %u\n", __func__, pos);

	if (!is_last_core()) {
		update_core_state(pos, 0);

		if (is_cluster0_off())
			ncore_caiu_offline(A53_CLUSTER0_CAIU);

		if (is_cluster1_off())
			ncore_caiu_offline(A53_CLUSTER1_CAIU);

		plat_secondary_cold_boot_setup();
	}

#if S32G_EMU == 0
	copy_bl31sram_image();
#endif
	platform_suspend(pos);

	/* Unreachable code */
	plat_panic_handler();
}

static void s32g_pwr_domain_suspend_finish(
					const psci_power_state_t *target_state)
{
	NOTICE("S32G TF-A: %s\n", __func__);
	gicv3_cpuif_enable(plat_my_core_pos());
}

static void s32g_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	NOTICE("S32G TF-A: %s\n", __func__);
}

static void s32g_get_sys_suspend_power_state(psci_power_state_t *req_state)
{
	int i;

	NOTICE("S32G TF-A: %s\n", __func__);

	/* FIXME revisit this, along with the power domain tree */
	/* CPU, cluster & system: off */
	for (i = MPIDR_AFFLVL0; i <= PLAT_MAX_PWR_LVL; i++)
		req_state->pwr_domain_state[i] = PLAT_MAX_OFF_STATE;
}

static void s32g_pwr_domain_suspend_pwrdown_early(
		const psci_power_state_t *target_state)
{
	NOTICE("S32G TF-A: %s\n", __func__);
}
#endif
static void s32_pwr_domain_off(const psci_power_state_t *target_state)
{
	NOTICE("S32 TF-A: %s\n", __func__);
}

static void __dead2 s32_system_reset(void)
{
	NOTICE("S32 TF-A: %s\n", __func__);
	s32_destructive_reset();
	plat_panic_handler();
}

static void __dead2 s32_system_off(void)
{
#if defined(PLAT_s32g2) || defined(PLAT_s32g3)
	pmic_system_off();
#endif
	plat_panic_handler();
}

static int32_t s32_migrate_info(u_register_t *resident_cpu)
{
	return PSCI_TOS_NOT_PRESENT_MP;
}

const spd_pm_ops_t s32_svc_pm = {
	.svc_migrate = NULL,
	.svc_migrate_info = s32_migrate_info,
};

const plat_psci_ops_t s32_psci_pm_ops = {
	/* cap: PSCI_CPU_OFF */
	.pwr_domain_off = s32_pwr_domain_off,
	/* cap: PSCI_CPU_ON_AARCH64 */
	.pwr_domain_on = s32_pwr_domain_on,
	.pwr_domain_on_finish = s32_pwr_domain_on_finish,
#if defined(PLAT_s32g2) || defined(PLAT_s32g3)
	/* cap: PSCI_CPU_SUSPEND_AARCH64 */
	.pwr_domain_suspend = s32g_pwr_domain_suspend,
	/* cap: PSCI_SYSTEM_SUSPEND_AARCH64 */
	.get_sys_suspend_power_state = s32g_get_sys_suspend_power_state,
	.pwr_domain_suspend_pwrdown_early =
					s32g_pwr_domain_suspend_pwrdown_early,
	.pwr_domain_suspend_finish = s32g_pwr_domain_suspend_finish,
	.pwr_domain_pwr_down_wfi = s32g_pwr_domain_pwr_down_wfi,
#endif
	.system_reset = s32_system_reset,
	.system_off = s32_system_off,
};

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	s32_warmboot_entry = sec_entrypoint;

	*psci_ops = &s32_psci_pm_ops;
	psci_register_spd_pm_hook(&s32_svc_pm);

	return 0;
}

const unsigned char *plat_get_power_domain_tree_desc(void)
{
	return s32_power_domain_tree_desc;
}

