/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <lib/psci/psci.h>
#include <drivers/arm/gicv3.h>
#include <plat/common/platform.h>
#include <s32n_bl_common.h>
#include <s32n_fss_scmi.h>

/* Might be used by plat_secondary_cold_boot_setup */
uintptr_t s32_warmboot_entry;

/** Executed by the primary core as part of the PSCI_CPU_ON call,
 *  e.g. during Linux kernel boot. The mpidr value represents the
 *  secondary core to be turned on.
 */
static int s32n_pwr_domain_on(u_register_t mpidr)
{
	int pos = plat_core_pos_by_mpidr(mpidr);
	int ret;

	if (pos < 0)
		return PSCI_E_INTERN_FAIL;

	NOTICE("S32N TF-A: %s: turning cpu %d on\n", __func__, pos);

	ret = fss_cpu_on(pos);
	if (ret)
		return PSCI_E_INVALID_PARAMS;

	/* Do some chores on behalf of the secondary core. ICC setup must be
	 * done by the secondaries, because the interface is not memory-mapped.
	 */
	gicv3_rdistif_init(pos);

	return PSCI_E_SUCCESS;
}

/** Executed by the woken (secondary) core after it exits the wfi holding pen
 *  during cold boot.
 */
static void s32n_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	unsigned int pos = plat_my_core_pos();

	gicv3_cpuif_enable(pos);

	NOTICE("S32N TF-A: %s: cpu %d running\n", __func__, pos);
}

/* Prepare to power off the calling CPU. */
static void s32n_pwr_domain_off(const psci_power_state_t *target_state)
{
	unsigned int pos = plat_my_core_pos();

	NOTICE("S32N TF-A: %s\n", __func__);

	gicv3_cpuif_disable(pos);
}

/** Invoke 'wfi' for the calling CPU and any platform specific operations.
 *  This handler should not return.
 */
static void __dead2 s32n_pwr_domain_pwr_down_wfi(const psci_power_state_t
						  *target_state)
{
	unsigned int pos = plat_my_core_pos();
	int ret;

	NOTICE("S32N TF-A: %s: cpu %u down\n", __func__, pos);

	ret = fss_cpu_off(pos);
	if (ret) {
		ERROR("Failed to turn off core: %u\n", pos);
		panic();
	}

	/* Should never reach here */
	psci_power_down_wfi();
	panic();
}

static void s32n_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	NOTICE("S32N TF-A: %s\n", __func__);
}

static void s32n_pwr_domain_suspend_finish(
					const psci_power_state_t *target_state)
{
	NOTICE("S32N TF-A: %s\n", __func__);
}

static void s32n_get_sys_suspend_power_state(psci_power_state_t *req_state)
{
	unsigned int i;

	NOTICE("S32N TF-A: %s\n", __func__);

	for (i = 0; i <= PLAT_MAX_PWR_LVL; ++i) {
		req_state->pwr_domain_state[i] = PLAT_MAX_OFF_STATE;
	}
}

static void __dead2 s32n_system_reset(void)
{
	ERROR("S32 TF-A: %s: operation not implemented.\n", __func__);
	panic();
}

static void __dead2 s32n_system_off(void)
{
	ERROR("S32 TF-A: %s: operation not implemented.\n", __func__);
	panic();
}

static const plat_psci_ops_t s32n_psci_ops = {
	.pwr_domain_on = s32n_pwr_domain_on,
	.pwr_domain_off = s32n_pwr_domain_off,
	.pwr_domain_on_finish = s32n_pwr_domain_on_finish,
	.pwr_domain_pwr_down_wfi = s32n_pwr_domain_pwr_down_wfi,
	/* PSCI_SYSTEM_SUSPEND_AARCH64 */
	.pwr_domain_suspend = s32n_pwr_domain_suspend,
	.pwr_domain_suspend_finish = s32n_pwr_domain_suspend_finish,
	.get_sys_suspend_power_state = s32n_get_sys_suspend_power_state,
	.system_reset = s32n_system_reset,
	.system_off = s32n_system_off,
};

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	int ret = 0;

	s32_warmboot_entry = sec_entrypoint;
	if (is_fss_used()) {
		ret = fss_set_cores_reset_addr(s32_warmboot_entry);
		if (ret) {
			ERROR("Could not configure AP cores reset address [ret = %d]\n", ret);
			panic();
		}
	}

	*psci_ops = &s32n_psci_ops;

	return ret;
}
