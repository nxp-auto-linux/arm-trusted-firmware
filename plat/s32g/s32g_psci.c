/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string.h>
#include <assert.h>
#include <common/debug.h>	/* printing macros such as INFO() */
#include <plat/common/platform.h>
#include "platform_def.h"

IMPORT_SYM(unsigned long, __BL31_START__, bl31_start);
IMPORT_SYM(unsigned long, __BL31_END__, bl31_end);

/* See firmware-design, psci-lib-integration-guide for details */
static uintptr_t warmboot_entry;

/* FIXME revisit tree composition */
static const unsigned char s32g_power_domain_tree_desc[] = {
	PLATFORM_SYSTEM_COUNT,
	PLATFORM_CLUSTER_COUNT,
	PLATFORM_CORE_COUNT / 2,
	PLATFORM_CORE_COUNT / 2
};

static int s32g_pwr_domain_on(u_register_t mpidr)
{
	NOTICE("S32G TF-A: %s\n", __func__);
	return PSCI_E_SUCCESS;
}

static void s32g_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	NOTICE("S32G TF-A: %s\n", __func__);
}

static void __dead2 s32g_pwr_domain_pwr_down_wfi(
					const psci_power_state_t *target_state)
{
	NOTICE("S32G TF-A: %s\n", __func__);

	/* S32G suspend to RAM is broadly equivalent to a system power off */
	psci_power_down_wfi();
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
	INFO("Copying self (0x%lx .. 0x%lx) to DRAM (0x%lx)...",
	     bl31_start, bl31_end, (unsigned long)S32G_PMEM_START);
	/* FIXME this may be too time-consuming; we should do it via DMA and/or
	 * copy only the relevant section(s) of the entire blob.
	 */
	_Static_assert(S32G_PMEM_END >= S32G_PMEM_START,
		       "S32G: PMEM_END < PMEM_START");
	/* TODO Ideally, this would have been a static assert, also; however,
	 * __BL31_START__ and __BL31_END__ are linker symbols, so static assert
	 * isn't possible.
	 */
	assert(bl31_end - bl31_start <= S32G_PMEM_END - S32G_PMEM_START);
	memcpy((unsigned char *)S32G_PMEM_START,
	       (unsigned char *)bl31_start,
	       bl31_end - bl31_start);
	puts(" Done.");
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
