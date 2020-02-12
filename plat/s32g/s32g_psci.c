/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch_helpers.h>
#include <bl31/bl31.h>		/* for bl31_warm_entrypoint() */
#include <bl31/interrupt_mgmt.h>
#include <string.h>
#include <assert.h>
#include <common/debug.h>	/* printing macros such as INFO() */
#include <plat/common/platform.h>
#include <drivers/arm/gicv3.h>

#include "s32g_ncore.h"
#include "s32g_mc_me.h"
#include "platform_def.h"

IMPORT_SYM(unsigned long, __BL31_START__, bl31_start);
IMPORT_SYM(unsigned long, __BL31_END__, bl31_end);

/* See firmware-design, psci-lib-integration-guide for details */
static uintptr_t warmboot_entry;

uint32_t s32g_core_release_var[PLATFORM_CORE_COUNT];

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

/** Executed by the running (primary) core as part of the PSCI_CPU_ON
 *  call, e.g. during Linux kernel boot.
 */
static int s32g_pwr_domain_on(u_register_t mpidr)
{
	int pos;

	pos = plat_core_pos_by_mpidr(mpidr);
	dsbsy();

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
	s32g_core_release_var[pos] = 1;
	flush_dcache_range((uintptr_t)&s32g_core_release_var[pos],
			   sizeof(s32g_core_release_var[pos]));
	plat_ic_raise_el3_sgi(S32G_SECONDARY_WAKE_SGI, mpidr);
	if (is_core_in_secondary_cluster(pos) &&
			!ncore_is_caiu_online(A53_CLUSTER1_CAIU))
		ncore_caiu_online(A53_CLUSTER1_CAIU);

	return PSCI_E_SUCCESS;
}

/** Executed by the woken (secondary) core after it exits the wfi holding pen.
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
}

/* Temp fixups to work around the fact that we are not really powering down
 * the SoC upon suspend (not yet). Place here all necessary fixups, so we can
 * easily revert them.
 */
static void s32g_pwr_down_wfi_fixups(void)
{
	disable_mmu_el3();
}

static void __dead2 s32g_pwr_domain_pwr_down_wfi(
					const psci_power_state_t *target_state)
{
	NOTICE("S32G TF-A: %s\n", __func__);

	/* S32G suspend to RAM is broadly equivalent to a system power off.
	 *
	 * Note: because the functional simulator does not support the wake up
	 * path via the external PMIC, we'll just simulate the CPU shutdown
	 * and instead *expect* to return from wfi rather than panicking as
	 * psci_power_down_wfi() does.
	 */

	s32g_pwr_down_wfi_fixups();

	/*
	 * A torn-apart variant of psci_power_down_wfi()
	 */
	dsb();
	wfi();
	bl31_warm_entrypoint();

	plat_panic_handler();
}

static void s32g_pwr_domain_suspend_finish(
					const psci_power_state_t *target_state)
{
	NOTICE("S32G TF-A: %s\n", __func__);
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
};

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	warmboot_entry = sec_entrypoint;

	*psci_ops = &s32g_psci_pm_ops;

	return 0;
}

const unsigned char *plat_get_power_domain_tree_desc(void)
{
	return s32g_power_domain_tree_desc;
}
