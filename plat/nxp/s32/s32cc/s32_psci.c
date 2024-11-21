/*
 * Copyright 2019-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "platform_def.h"
#include "s32cc_lowlevel.h"
#include "s32cc_ncore.h"
#include "s32cc_plat_funcs.h"
#include "s32cc_pmic.h"

#if defined(PLAT_s32g2) || defined(PLAT_s32g3)
#include <lib/mmio.h>
#include "s32g_bl_common.h"
#include "s32g_mc_me.h"
#else
#include "s32cc_bl_common.h"
#include "s32cc_mc_me.h"
#endif

#include <arch_helpers.h>
#include <assert.h>
#include <common/debug.h>	/* printing macros such as INFO() */
#include <drivers/arm/gicv3.h>
#include <plat/common/platform.h>
#include <s32cc_scp_scmi.h>

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
	int ret;
	uintptr_t core_start_addr = (uintptr_t)&plat_secondary_cold_boot_setup;

	pos = plat_core_pos_by_mpidr(mpidr);
	if (pos < 0)
		return PSCI_E_INTERN_FAIL;

	dsbsy();

	if (is_scp_used()) {
		ret = scp_cpu_on(pos);
		if (ret)
			return PSCI_E_INVALID_PARAMS;
	} else {
		if (is_a53_core_in_reset(pos)) {
			s32_set_core_entrypoint(pos, core_start_addr);
			s32_kick_secondary_ca53_core(pos);
		} else {
			update_core_state(pos, CPU_USE_WFI_FOR_SLEEP,
					  CPU_USE_WFI_FOR_SLEEP);
		}
	}

	/* Do some chores on behalf of the secondary core. ICC setup must be
	 * done by the secondaries, because the interface is not memory-mapped.
	 */
	gicv3_rdistif_init(pos);

	/* Kick the secondary core out of wfi */
	NOTICE("S32 TF-A: %s: booting up core %d (%u)\n", __func__, pos,
	       get_core_state(pos, CPU_USE_WFI_FOR_SLEEP));

	if (is_core_in_secondary_cluster(pos) &&
	    !ncore_is_caiu_online(A53_CLUSTER1_CAIU))
		ncore_caiu_online(A53_CLUSTER1_CAIU);

	if (!is_core_in_secondary_cluster(pos) &&
	    !ncore_is_caiu_online(A53_CLUSTER0_CAIU))
		ncore_caiu_online(A53_CLUSTER0_CAIU);

	update_core_state(pos, CPU_ON, CPU_ON);

	/* Wait GIC initialization */
	while (get_core_state(pos, CPUIF_EN | CPU_ON) != (CPUIF_EN | CPU_ON));

	/* Send an interrupt if the core is waiting in a WFI loop */
	if (get_core_state(pos, CPU_USE_WFI_FOR_SLEEP)) {
		plat_ic_raise_el3_sgi(S32_SECONDARY_WAKE_SGI, mpidr);
	}

	return PSCI_E_SUCCESS;
}

static void sleep_wfi_loop(void)
{
	u_register_t scr;
	unsigned int pos = plat_my_core_pos();
	unsigned int intid;

	/* GICR_IGROUPR0, GICR_IGRPMOD0 */
	gicv3_set_interrupt_group(S32_SECONDARY_WAKE_SGI, pos, INTR_GROUP0);
	/* GICR_ISENABLER0 */
	assert(plat_ic_is_sgi(S32_SECONDARY_WAKE_SGI));
	gicv3_enable_interrupt(S32_SECONDARY_WAKE_SGI, pos);

	if (!get_core_state(pos, CPUIF_EN)) {
		gicv3_cpuif_enable(pos);
		update_core_state(pos, CPUIF_EN, CPUIF_EN);
	}

	scr = read_scr_el3();

	/* Make sure interrupts are taken to EL3 before going into wfi */
	do {
		write_scr_el3(scr | SCR_IRQ_BIT | SCR_FIQ_BIT);
		isb();
		dsb();
		wfi();
		write_scr_el3(scr);

		/* Restore SCR_EL3 */
		intid = gicv3_get_pending_interrupt_id();
		if (intid < MAX_SPI_ID) {
			/* Mark it as consumed */
			gicv3_clear_interrupt_pending(intid, pos);
		}

		if (is_core_enabled(pos) && intid == S32_SECONDARY_WAKE_SGI) {
			break;
		}
	} while (true);

	gicv3_disable_interrupt(S32_SECONDARY_WAKE_SGI, pos);
}

/** Executed by the woken (secondary) core after it exits the wfi holding pen
 *  during cold boot.
 */
static void s32_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	unsigned int pos = plat_my_core_pos();
	NOTICE("S32 TF-A: %s: cpu %d running\n", __func__, pos);

	update_core_state(pos, CPU_USE_WFI_FOR_SLEEP, 0);
	if (!get_core_state(pos, CPUIF_EN)) {
		gicv3_cpuif_enable(pos);
		update_core_state(pos, CPUIF_EN, CPUIF_EN);
	}

#if (S32_SAVE_CNTVCT == 1) && (S32_BL33_AT_EL2 == 1)
	init_cntvoff_el2();
#endif
}

#if defined(PLAT_s32g2) || defined(PLAT_s32g3)
static void s32g_pwr_domain_suspend_finish(
					const psci_power_state_t *target_state)
{
	unsigned int pos = plat_my_core_pos();

	NOTICE("S32G TF-A: %s\n", __func__);
	plat_gic_restore();

	gicv3_cpuif_enable(pos);
	update_core_state(pos, CPUIF_EN | CPU_ON, CPUIF_EN | CPU_ON);

#if (S32_SAVE_CNTVCT == 1) && (S32_BL33_AT_EL2 == 1)
	init_cntvoff_el2();
#endif
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

static void __dead2 s32_pwr_domain_pwr_down_wfi(
					const psci_power_state_t *target_state)
{
	unsigned int pos = plat_my_core_pos();
	bool last_core = is_last_core();
	int ret;

	NOTICE("S32 TF-A: %s: cpu = %u\n", __func__, pos);

	/* Mark the core as offline */
	update_core_state(pos, CPUIF_EN | CPU_ON, 0);
	gicv3_cpuif_disable(pos);

	if (!last_core) {
		if (is_cluster0_off()) {
			ncore_caiu_offline(A53_CLUSTER0_CAIU);
		}

		if (is_cluster1_off()) {
			ncore_caiu_offline(A53_CLUSTER1_CAIU);
		}

		if (is_scp_used()) {
			ret = scp_cpu_off(pos);
			if (ret) {
				ERROR("Failed to turn off core %u\n", pos);
				plat_panic_handler();
			}
		}
		sleep_wfi_loop();
		plat_secondary_cold_boot_setup();
	}

	/* The ARM Generic Timer Virtual Counter value is saved only during
	 * system suspend, by the last core.
	 */
	plat_generic_timer_save();
	s32_plat_suspend(pos);

	/* Unreachable code */
	plat_panic_handler();
}

static void s32_pwr_domain_off(const psci_power_state_t *target_state)
{
	NOTICE("S32 TF-A: %s\n", __func__);
}

static void __dead2 s32_system_reset(void)
{
	NOTICE("S32 TF-A: %s\n", __func__);

	if (is_scp_used()) {
		scp_reset_platform();
	} else {
		s32_destructive_reset();
	}
	plat_panic_handler();
}

static void __dead2 s32_system_off(void)
{
	if (is_scp_used()) {
		scp_shutdown_platform();
	} else {
		pmic_system_off();
	}
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
	.pwr_domain_pwr_down_wfi = s32_pwr_domain_pwr_down_wfi,
#if defined(PLAT_s32g2) || defined(PLAT_s32g3)
	/* cap: PSCI_CPU_SUSPEND_AARCH64 */
	.pwr_domain_suspend = s32g_pwr_domain_suspend,
	/* cap: PSCI_SYSTEM_SUSPEND_AARCH64 */
	.get_sys_suspend_power_state = s32g_get_sys_suspend_power_state,
	.pwr_domain_suspend_pwrdown_early =
					s32g_pwr_domain_suspend_pwrdown_early,
	.pwr_domain_suspend_finish = s32g_pwr_domain_suspend_finish,
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

