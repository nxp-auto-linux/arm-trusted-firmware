/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "platform_def.h"
#include "pmic/vr5510.h"
#include "s32g_clocks.h"
#include "s32g_lowlevel.h"	/* plat_is_my_cpu_primary() */
#include "s32g_mc_me.h"
#include "s32g_ncore.h"

#include <arch_helpers.h>
#include <assert.h>
#include <bl31/bl31.h>		/* for bl31_warm_entrypoint() */
#include <bl31/interrupt_mgmt.h>
#include <common/debug.h>	/* printing macros such as INFO() */
#include <drivers/arm/gicv3.h>
#include <lib/mmio.h>
#include <plat/common/platform.h>
#include <string.h>

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

/* Temp fixups to work around the fact that we are not really powering down
 * the SoC upon suspend (not yet). Place here all necessary fixups, so we can
 * easily revert them.
 *
 * This should only be executed by the primary core, the one expecting
 * to take the wake-up interrupt.
 */
static void s32g_pwr_down_wfi_fixups(void)
{
	disable_mmu_el3();

	/* The primary core needs to wait for the RTC interrupt. The secondary
	 * cores need a similar configuration, but for the wakeup SGI.
	 * This all assumes the RTC is routed *by PE*, to the primary core.A
	 *
	 * Also mask the exception in D,A,I,F so we wake from wfi without
	 * having to handle the exception immediately.
	 */
	if (plat_is_my_cpu_primary()) {
		write_scr_el3(read_scr_el3() | SCR_FIQ_BIT);
		disable_fiq();
	} else {
		write_scr_el3(read_scr_el3() | SCR_IRQ_BIT);
		disable_irq();
	}
}

static void s32g_rtc_acknowledge_irq(void)
{
	mmio_write_32(S32G_RTC_BASE + RTC_RTCS_OFFSET, RTC_RTCS_RTCF);
}

static void s32g_primary_resume_post_wfi_fixups(void)
{
	/* Clear and acknowledge RTC interrupt that woke us up */
	s32g_rtc_acknowledge_irq();
	gicv3_acknowledge_interrupt();
	/* ICC_CTLR_EL3[EOImode_EL3] is 0, interrupt is both priority-dropped
	 * and deactivated.
	 */
	gicv3_end_of_interrupt(S32G_RTC_INT);

	/* Wake up the secondaries, which are still in wfi */
	NOTICE("S32G TF-A: waking up secondaries...\n");
	plat_ic_raise_el3_sgi(S32G_SECONDARY_WAKE_SGI, 0x80000001);
	plat_ic_raise_el3_sgi(S32G_SECONDARY_WAKE_SGI, 0x80000100);
	plat_ic_raise_el3_sgi(S32G_SECONDARY_WAKE_SGI, 0x80000101);
}

static void s32g_secondary_resume_post_wfi_fixups(void)
{
	const unsigned int intid = S32G_SECONDARY_WAKE_SGI;
	int pos = plat_my_core_pos();

	gicv3_clear_interrupt_pending(intid, pos);

	write_scr_el3(read_scr_el3() & ~SCR_IRQ_BIT);
	/* disable NS interrupts until the kernel is ready to process them */
	write_icc_igrpen1_el3(read_icc_igrpen1_el3() &
			      ~IGRPEN1_EL3_ENABLE_G1NS_BIT);

	return;
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

	/* Set standby master core and request the standby transition */
	s32g_set_stby_master_core(S32G_STBY_MASTER_PART, S32G_STBY_MASTER_CORE);

	/*
	 * A torn-apart variant of psci_power_down_wfi()
	 */
	dsb();
	wfi();

	if (plat_is_my_cpu_primary())
		s32g_primary_resume_post_wfi_fixups();
	else {
		s32g_secondary_resume_post_wfi_fixups();
		/* FIXME temporarily prevent the secondary core from
		 * progressing; as it stands, it will successfully execute
		 * inside the kernel space, but will crash eventually.
		 */
		asm volatile ("b	.");
	}

	bl31_warm_entrypoint();
	plat_panic_handler();
}

static void s32g_pwr_domain_suspend_finish(
					const psci_power_state_t *target_state)
{
	NOTICE("S32G TF-A: %s\n", __func__);
}

static int prepare_vr5510(void)
{
	int ret;
	vr5510_t mu, fsu;

	uint16_t reg;
	uint8_t *regp = (uint8_t *)&reg;

	ret = vr5510_get_inst(VR5510_MU_NAME, &mu);
	if (ret)
		return ret;

	ret = vr5510_get_inst(VR5510_FSU_NAME, &fsu);
	if (ret)
		return ret;


	/* Clear I2C errors if any */
	reg = VR5510_FLAG3_I2C_M_REQ | VR5510_FLAG3_I2C_M_CRC;
	ret = vr5510_write(mu, VR5510_M_FLAG3, regp, sizeof(reg));
	if (ret)
		return ret;

	/* Wait forever */
	reg = 0x0;
	ret = vr5510_write(mu, VR5510_M_SM_CTRL1, regp, sizeof(reg));
	if (ret)
		return ret;

	reg = VR5510_CTRL3_VPREV_STBY | VR5510_CTRL3_HVLDO_STBY
		| VR5510_CTRL3_BUCK3_STBY |  VR5510_CTRL3_LDO2_STBY;
	ret = vr5510_write(mu, VR5510_M_REG_CTRL3, regp, sizeof(reg));
	if (ret)
		return ret;

	reg = VR5510_FLAG1_ALL_FLAGS;
	ret = vr5510_write(mu, VR5510_M_FLAG1, regp, sizeof(reg));
	if (ret)
		return ret;

	reg = VR5510_FLAG2_ALL_FLAGS;
	ret = vr5510_write(mu, VR5510_M_FLAG2, regp, sizeof(reg));
	if (ret)
		return ret;

	reg = VR5510_M_CLOCK2_600KHZ;
	ret = vr5510_write(mu, VR5510_M_CLOCK2, regp, sizeof(reg));
	if (ret)
		return ret;

	/* Check for I2C errors */
	ret = vr5510_read(mu, VR5510_M_FLAG3, regp, sizeof(reg));
	if (ret)
		return ret;

	if (reg & (VR5510_FLAG3_I2C_M_REQ | VR5510_FLAG3_I2C_M_CRC)) {
		ERROR("VR5510-MU: Detected I2C errors");
		return -EIO;
	}

	/* Clear I2C errors if any */
	reg = VR5510_GRL_FLAGS_I2C_FS_REQ | VR5510_GRL_FLAGS_I2C_FS_CRC;
	ret = vr5510_write(fsu, VR5510_FS_GRL_FLAGS, regp, sizeof(reg));
	if (ret)
		return ret;

	/* Disable I2C timeout */
	reg = 0;
	ret = vr5510_write(fsu, VR5510_FS_I_SAFE_INPUTS, regp, sizeof(reg));
	if (ret)
		return ret;

	reg = VR5510_FS_I_NOT_VALUE(reg);
	ret = vr5510_write(fsu, VR5510_FS_I_NOT_SAFE_INPUTS, regp, sizeof(reg));
	if (ret)
		return ret;

	/* Check for I2C errors */
	ret = vr5510_read(fsu, VR5510_FS_GRL_FLAGS, regp, sizeof(reg));
	if (ret)
		return ret;

	if (reg & (VR5510_GRL_FLAGS_I2C_FS_REQ | VR5510_GRL_FLAGS_I2C_FS_CRC)) {
		ERROR("VR5510-FSU: Detected I2C errors\n");
		return -EIO;
	}

	/* Standby request */
	ret = vr5510_read(fsu, VR5510_FS_SAFE_IOS, regp, sizeof(reg));
	if (ret)
		return ret;

	reg |= VR5510_SAFE_IOS_STBY_REQ;
	ret = vr5510_write(fsu, VR5510_FS_SAFE_IOS, regp, sizeof(reg));
	if (ret)
		return ret;

	return 0;
}

static void s32g_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	NOTICE("S32G TF-A: %s\n", __func__);
	prepare_vr5510();

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
