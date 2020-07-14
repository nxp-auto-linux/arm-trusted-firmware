/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "bl31_sram.h"
#include "platform_def.h"
#include "s32g_clocks.h"
#include "s32g_lowlevel.h"	/* plat_is_my_cpu_primary() */
#include "s32g_mc_me.h"
#include "s32g_ncore.h"
#include "ssram_mailbox.h"
#include "s32g_resume.h"

#include <arch_helpers.h>
#include <assert.h>
#include <bl31/interrupt_mgmt.h>
#include <common/debug.h>	/* printing macros such as INFO() */
#include <drivers/arm/gicv3.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <plat/common/platform.h>
#include <string.h>

IMPORT_SYM(unsigned long, __BL31_START__, bl31_start);
IMPORT_SYM(unsigned long, __BL31_END__, bl31_end);

/* See firmware-design, psci-lib-integration-guide for details */
/* Used by plat_secondary_cold_boot_setup */
uintptr_t s32g_warmboot_entry;

volatile uint32_t s32g_core_release_var[PLATFORM_CORE_COUNT];

/* FIXME revisit tree composition */
static const unsigned char s32g_power_domain_tree_desc[] = {
	PLATFORM_SYSTEM_COUNT,
	PLATFORM_CLUSTER_COUNT,
	PLATFORM_CORE_COUNT / 2,
	PLATFORM_CORE_COUNT / 2
};

static bool is_core_in_secondary_cluster(int pos)
{
	return (pos == 2 || pos == 3);
}

static void update_core_state(uint32_t core, uint32_t state)
{
	s32g_core_release_var[core] = state;
	flush_dcache_range((uintptr_t)&s32g_core_release_var[core],
			   sizeof(s32g_core_release_var[core]));
}

/** Executed by the primary core as part of the PSCI_CPU_ON call,
 *  e.g. during Linux kernel boot.
 */
static int s32g_pwr_domain_on(u_register_t mpidr)
{
	int pos;
	uintptr_t core_start_addr = (uintptr_t)&plat_secondary_cold_boot_setup;

	pos = plat_core_pos_by_mpidr(mpidr);
	dsbsy();

	if (s32g_core_in_reset(pos))
		s32g_kick_secondary_ca53_core(pos, core_start_addr);
	else
		return 0;

	/* TODO: this sequence should be revisited for full cpu hotplug support
	 * (i.e. turning on/off cpus in an arbitrary order). For now, it only
	 * works at boot.
	 */

	/* Do some chores on behalf of the secondary core. ICC setup must be
	 * done by the secondaries, because the interface is not memory-mapped.
	 */
	gicv3_rdistif_init(pos);
	/* GICR_IGROUPR0, GICR_IGRPMOD0 */
	gicv3_set_interrupt_type(S32G_SECONDARY_WAKE_SGI, pos, INTR_GROUP0);
	/* GICR_ISENABLER0 */
	assert(plat_ic_is_sgi(S32G_SECONDARY_WAKE_SGI));
	gicv3_enable_interrupt(S32G_SECONDARY_WAKE_SGI, pos);

	/* Kick the secondary core out of wfi */
	NOTICE("S32G TF-A: %s: booting up core %d\n", __func__, pos);
	update_core_state(pos, 1);
	plat_ic_raise_el3_sgi(S32G_SECONDARY_WAKE_SGI, mpidr);
	if (is_core_in_secondary_cluster(pos) &&
			!ncore_is_caiu_online(A53_CLUSTER1_CAIU))
		ncore_caiu_online(A53_CLUSTER1_CAIU);

	return PSCI_E_SUCCESS;
}

/** Executed by the woken (secondary) core after it exits the wfi holding pen
 *  during cold boot.
 */
static void s32g_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	int pos;
	unsigned int intid;

	NOTICE("S32G TF-A: %s: cpu %d running\n", __func__, plat_my_core_pos());

	/* Clear pending interrupt */
	pos = plat_my_core_pos();
	while ((intid = gicv3_get_pending_interrupt_id()) <= MAX_SPI_ID) {
		if (intid != S32G_SECONDARY_WAKE_SGI)
			WARN("%s(): Interrupt %d found pending instead of the expected %d\n",
			     __func__, intid, S32G_SECONDARY_WAKE_SGI);
		gicv3_clear_interrupt_pending(intid, pos);
	}

	write_scr_el3(read_scr_el3() & ~SCR_IRQ_BIT);
}

static void copy_bl31sram_image(void)
{
	uint32_t npages;
	int ret;
	static bool copied;

	if (copied)
		return;

	/* Copy bl31 sram stage */
	memcpy((void *)BL31SRAM_BASE, bl31sram, bl31sram_len);
	npages = bl31sram_len / PAGE_SIZE;
	if (bl31sram_len % PAGE_SIZE)
		npages++;

	ret = xlat_change_mem_attributes(BL31SRAM_BASE, npages * PAGE_SIZE,
			MT_CODE | MT_SECURE);
	if (ret)
		ERROR("Failed to change the attributes of BL31 SRAM memory\n");

	copied = true;
}

static void bl31sram_entry(void)
{
	bl31_sram_entry_t entry;

	entry = (void *)BL31SRAM_BASE;
	entry();
}

static void set_warm_entry(void)
{
	uintptr_t warm_entry;

	warm_entry = BL31SSRAM_MAILBOX + offsetof(struct s32g_ssram_mailbox,
						  bl31_warm_entrypoint);
	mmio_write_64(warm_entry, (uintptr_t)s32g_resume_entrypoint);
}

static void __dead2 s32g_pwr_domain_pwr_down_wfi(
					const psci_power_state_t *target_state)
{
	unsigned int pos = plat_my_core_pos();
	unsigned int i;

	NOTICE("S32G TF-A: %s: cpu = %u\n", __func__, pos);

	copy_bl31sram_image();

	if (!plat_is_my_cpu_primary()) {
		update_core_state(pos, 0);
		plat_secondary_cold_boot_setup();

		/* Unreachable code */
		plat_panic_handler();
	}

	for (i = 0; i < PLATFORM_CORE_COUNT; i++) {
		gicv3_cpuif_disable(i);
	}

	plat_gic_save();
	set_warm_entry();
	prepare_pmic();

	/* Shutting down cores */
	/* M7 cores */
	s32g_turn_off_core(S32G_MC_ME_CM7_PART, 2);
	s32g_turn_off_core(S32G_MC_ME_CM7_PART, 1);
	s32g_turn_off_core(S32G_MC_ME_CM7_PART, 0);

	/* A53 cores */
	s32g_turn_off_core(S32G_MC_ME_CA53_PART, 3);
	s32g_turn_off_core(S32G_MC_ME_CA53_PART, 2);
	s32g_turn_off_core(S32G_MC_ME_CA53_PART, 1);

	/* PFE blocks */
	s32g_disable_cofb_clk(S32G_MC_ME_PFE_PART, 0);
	/* Keep the DDR clock */
	s32g_disable_cofb_clk(S32G_MC_ME_USDHC_PART,
			      S32G_MC_ME_PRTN_N_REQ(S32G_MC_ME_DDR_0_REQ));

	/* Switching all MC_CGM muxes to FIRC */
	s32g_sw_clks2firc();

	/* Turn off DFS */
	s32g_disable_dfs(S32G_PERIPH_DFS);
	s32g_disable_dfs(S32G_CORE_DFS);

	/* Turn off PLL */
	s32g_disable_pll(S32G_ACCEL_PLL, 2);
	s32g_disable_pll(S32G_PERIPH_PLL, 8);
	s32g_disable_pll(S32G_CORE_PLL, 2);

	bl31sram_entry();
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

static void s32g_pwr_domain_off(const psci_power_state_t *target_state)
{
	NOTICE("S32G TF-A: %s\n", __func__);
}

void __dead2 s32g_system_reset(void)
{
	NOTICE("S32G TF-A: %s\n", __func__);
	s32g_destructive_reset();
	plat_panic_handler();
}

const plat_psci_ops_t s32g_psci_pm_ops = {
	/* cap: PSCI_CPU_OFF */
	.pwr_domain_off = s32g_pwr_domain_off,
	/* cap: PSCI_CPU_ON_AARCH64 */
	.pwr_domain_on = s32g_pwr_domain_on,
	.pwr_domain_on_finish = s32g_pwr_domain_on_finish,
	/* cap: PSCI_CPU_SUSPEND_AARCH64 */
	.pwr_domain_suspend = s32g_pwr_domain_suspend,
	/* cap: PSCI_SYSTEM_SUSPEND_AARCH64 */
	.get_sys_suspend_power_state = s32g_get_sys_suspend_power_state,
	.pwr_domain_suspend_pwrdown_early =
					s32g_pwr_domain_suspend_pwrdown_early,
	.pwr_domain_suspend_finish = s32g_pwr_domain_suspend_finish,
	.pwr_domain_pwr_down_wfi = s32g_pwr_domain_pwr_down_wfi,
	.system_reset = s32g_system_reset,
};

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	s32g_warmboot_entry = sec_entrypoint;

	*psci_ops = &s32g_psci_pm_ops;

	return 0;
}

const unsigned char *plat_get_power_domain_tree_desc(void)
{
	return s32g_power_domain_tree_desc;
}