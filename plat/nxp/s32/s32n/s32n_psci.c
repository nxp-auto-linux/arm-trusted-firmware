/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <lib/psci/psci.h>
#include <drivers/arm/gicv3.h>
#include <dt-bindings/clock/s32n79-clock-freq.h>
#include <dt-bindings/power/s32n-scmi-pd.h>
#include <plat/common/platform.h>
#include <s32n_bl_common.h>
#include <s32n_fss_scmi.h>
#include <s32n_pl011.h>
#include <s32n_plat_funcs.h>

#define CLUSTER_PWR_STATE(state) ((state)->pwr_domain_state[S32N_PWR_LVL1])
#define SYSTEM_PWR_STATE(state) ((state)->pwr_domain_state[S32N_PWR_LVL2])

/* Might be used by plat_secondary_cold_boot_setup */
uintptr_t s32_warmboot_entry;

static void s32n_gic_cluster_pwr_dwn(bool cluster_off)
{
	gicv3_cpuif_disable(plat_my_core_pos());

	if (cluster_off) {
		write_clusterpwrdn(DSU_CLUSTER_PWR_OFF);
	}
}

static void __dead2 s32n_last_core_pwr_dwn_wfi(void)
{
	s32n_gic_cluster_pwr_dwn(true);

	/*
	 * Arm CPUPWRCTLR_EL1.CORE_PWRDN_EN so this core powers down on wfi.
	 * FSS only completes the cohort shutdown/reset once every AP core,
	 * including this last online one, has armed it and entered wfi.
	 */
	psci_pwrdown_cpu_start(PSCI_CPU_PWR_LVL);

	while (true) {
		/* power down when executing wfi as the last core */
		wfi();
	}
}

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

	return PSCI_E_SUCCESS;
}

/** Executed by the woken (secondary) core after it exits the wfi holding pen
 *  during cold boot.
 */
static void s32n_pwr_domain_on_finish(const psci_power_state_t *target_state)
{
	unsigned int pos = plat_my_core_pos();

	gicv3_rdistif_init(pos);
	gicv3_cpuif_enable(pos);

	NOTICE("S32N TF-A: %s: cpu %d running\n", __func__, pos);
}

/* Prepare to power off the calling CPU. */
static void s32n_pwr_domain_off(const psci_power_state_t *target_state)
{
	NOTICE("S32N TF-A: %s\n", __func__);

	s32n_gic_cluster_pwr_dwn(is_local_state_off(CLUSTER_PWR_STATE(target_state)));
}

/** Invoke 'wfi' for the calling CPU and any platform specific operations.
 *  This handler should not return.
 */
static void __dead2 s32n_pwr_domain_pwr_down_wfi(const psci_power_state_t
						  *target_state)
{
	unsigned int pos = plat_my_core_pos();

	NOTICE("S32N TF-A: %s: cpu %u down\n", __func__, pos);

	if (is_local_state_off(SYSTEM_PWR_STATE(target_state))) {
		/* Issue a system suspend from the last online core */
		if (fss_system_suspend() != 0) {
			panic();
		}

		/* Handshake with the FSS as the last online core. */
		s32n_last_core_pwr_dwn_wfi();
	}

	fss_cpu_off(pos);
}

static void s32n_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	NOTICE("S32N TF-A: %s\n", __func__);

	s32n_gic_cluster_pwr_dwn(is_local_state_off(CLUSTER_PWR_STATE(target_state)));

	s32n_plat_gic_save(is_local_state_off(SYSTEM_PWR_STATE(target_state)));
}

static void s32n_pwr_domain_suspend_finish(
					const psci_power_state_t *target_state)
{
	int ret;

	if (is_local_state_off(SYSTEM_PWR_STATE(target_state))) {
		/* this is a resume */
		console_s32n_register(GR_CLK_COSS_PERIPH_DIV0_RATE);

		/*
		 * The cohort power cycle clears CNTFRQ_EL0 and the generic PSCI
		 * code restores it only after this handler returns. Restore it
		 * here, before the code below arms any timeout.
		 */
		write_cntfrq_el0((u_register_t)plat_get_syscnt_freq2());
		isb();

		/* FSS reinitializes the SCMI communication framework after resume. */
		ret = fss_handshake_init(true);
		if (ret)
			panic();

		ret = fss_set_cores_reset_addr(s32_warmboot_entry);
		if (ret)
			panic();
	}

	s32n_plat_gic_restore(is_local_state_off(SYSTEM_PWR_STATE(target_state)));
	gicv3_cpuif_enable(plat_my_core_pos());
}

static int s32n_get_node_hw_state(u_register_t mpidr, unsigned int power_level)
{
	int pos = plat_core_pos_by_mpidr(mpidr);
	unsigned int state;
	int ret;

	if ((pos < 0) || (pos >= PLATFORM_CORE_COUNT))
		return PSCI_E_INVALID_PARAMS;

	/* System power state is not supported */
	if (power_level == S32N_PWR_LVL2)
		return PSCI_E_NOT_SUPPORTED;

	ret = fss_get_node_hw_state(pos, power_level, &state);
	if (ret)
		return PSCI_E_INVALID_PARAMS;

	switch (state) {
	case S32N_SCMI_PD_LEVEL_ON:
		return HW_ON;
	case S32N_SCMI_PD_LEVEL_RET:
	case S32N_SCMI_PD_LEVEL_SLEEP:
		return HW_STANDBY;
	case S32N_SCMI_PD_LEVEL_OFF:
		return HW_OFF;
	default:
		return PSCI_E_INVALID_PARAMS;
	}
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
	if (fss_cohort_reset() != 0) {
		panic();
	}

	/* Handshake with the FSS as the last online core. */
	s32n_last_core_pwr_dwn_wfi();
}

static void __dead2 s32n_system_off(void)
{
	if (fss_cohort_shutdown() != 0) {
		panic();
	}

	/* Handshake with the FSS as the last online core. */
	s32n_last_core_pwr_dwn_wfi();
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
	.get_node_hw_state = s32n_get_node_hw_state,
	.system_reset = s32n_system_reset,
	.system_off = s32n_system_off,
};

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	int ret = 0;

	s32_warmboot_entry = sec_entrypoint;
	ret = fss_set_cores_reset_addr(s32_warmboot_entry);
	if (ret) {
		ERROR("Could not configure AP cores reset address [ret = %d]\n", ret);
		panic();
	}

	*psci_ops = &s32n_psci_ops;

	return ret;
}
