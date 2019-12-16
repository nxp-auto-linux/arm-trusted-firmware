/*
 * Copyright 2019 NXP
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

/** Executed by the running (primary) core as part of the PSCI_CPU_ON
 *  call, e.g. during Linux kernel boot.
 */
static int s32g_pwr_domain_on(u_register_t mpidr)
{
	int pos;

	pos = plat_core_pos_by_mpidr(mpidr);
	s32g_core_release_var[pos] = 1;
	dsbsy();

	/* Do some chores on behalf of the secondary core. ICC setup must be
	 * done by the secondaries, because the interface is not memory-mapped.
	 */
	gicv3_rdistif_init(pos);
	/* GICR_IGROUPR0, GICR_IGRPMOD0 */
	gicv3_set_interrupt_type(S32G_SECONDARY_WAKE_SGI, pos, INTR_GROUP0);
	/* GICR_ISENABLER0 */
	gicv3_enable_interrupt(S32G_SECONDARY_WAKE_SGI, pos);

	/* Kick the secondary core out of wfi */
	NOTICE("S32G TF-A: %s: booting up core %d\n", __func__, pos);
	plat_ic_raise_el3_sgi(S32G_SECONDARY_WAKE_SGI, mpidr);

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

const plat_psci_ops_t s32g_psci_pm_ops = {
	/* cap: PSCI_CPU_OFF */
	.pwr_domain_off = NULL,
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


/*
 * Copy the PSCI callbacks (in fact, the entire bl31 binary) to the protected
 * DRAM area only accessible to privileged contexts.
 * This must be called before XRDC is set up, since after that it becomes
 * read-only (even to privileged code) until the next reboot.
 */
void s32g_psci_move_to_pram(void)
{
	/* DDR is not initialized, and besides we won't do relocation this way
	 * anymore, but instead will seek to deploy BL31 entirely in DDR
	 */
	WARN("Skipping %s(); will need to revisit it.", __func__);
#if 0
	INFO("Copying self (0x%lx .. 0x%lx) to DRAM (0x%lx)...",
	     bl31_start, bl31_end, (unsigned long)S32G_PMEM_START);
	/* FIXME this may be too time-consuming; we should do it via DMA and/or
	 * copy only the relevant section(s) of the entire blob.
	 */
	_Static_assert(S32G_PMEM_END >= S32G_PMEM_START,
		       "S32G: PMEM_END < PMEM_START");
	/* Ideally, this would have been a static assert; however,
	 * __BL31_START__ and __BL31_END__ are linker symbols, so static assert
	 * isn't possible.
	 */
	assert(bl31_end - bl31_start <= S32G_PMEM_END - S32G_PMEM_START);
	memcpy((unsigned char *)S32G_PMEM_START,
	       (unsigned char *)bl31_start,
	       bl31_end - bl31_start);
	puts(" Done.");
#endif
}

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
