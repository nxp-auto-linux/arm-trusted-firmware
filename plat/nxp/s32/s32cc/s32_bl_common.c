/*
 * Copyright 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch/aarch64/arch.h>
#include <common/debug.h>
#include <drivers/generic_delay_timer.h>
#include "ddr_lp.h"
#include "ddr_utils.h"
#include <libfdt.h>
#include <lib/mmio.h>
#include <inttypes.h>
#include "platform_def.h"
#include "s32cc_bl_common.h"
#include "s32cc_clocks.h"
#include <dt-bindings/clock/s32gen1-clock-freq.h>
#if (ERRATA_S32_050543 == 1)
#include <dt-bindings/ddr-errata/s32-ddr-errata.h>
#include "s32cc_ddr_errata_funcs.h"
#endif
#include "s32cc_dt.h"
#include "s32cc_lowlevel.h"
#include "s32cc_ncore.h"
#include "s32cc_pinctrl.h"
#include "s32cc_scp_utils.h"
#if TRUSTED_BOARD_BOOT
#include <drivers/nxp/s32/hse/hse_core.h>
#endif

#include "s32cc_bl2_el3.h"

struct s32_i2c_driver i2c_drivers[S32_MAX_I2C_MODULES];
size_t i2c_fill_level;

bool is_lockstep_enabled(void)
{
	bool lockstep_en = false;
	int ret;

	if (!is_scp_used()) {
		if (mmio_read_32(GPR_BASE_ADDR + GPR06_OFF) & CA53_LOCKSTEP_EN)
			return true;

		return false;
	}

	ret = scp_is_lockstep_enabled(&lockstep_en);
	if (ret) {
		ERROR("Failed to get lockstep enabled status from SCP\n");
		return false;
	}

	return lockstep_en;
}

#if (ERRATA_S32_050543 == 1)
void ddr_errata_update_flag(uint8_t flag)
{
	mmio_write_32(DDR_ERRATA_REGION_BASE, flag);
}
#endif

#ifdef HSE_SUPPORT
void wait_hse_init(void)
{
	/*
	 * if HSE FW is present, write HSE_CONFIG_PERIPH_DONE
	 * to signal clock config is done
	 */
	mmio_write_32(HSE_MU0_GCR, LOCAL_HSE_HOST_PERIPH_CONFIG_DONE);

	/*
	 * if HSE FW is present, wait until HSE FW signals
	 * that init is complete - HSE_STATUS_INIT_OK
	 */
	while ((mmio_read_32(HSE_MU0_FSR) & LOCAL_HSE_STATUS_INIT_OK) == 0)
		;
}
#endif

#if TRUSTED_BOARD_BOOT
void hse_secboot_setup(void)
{
	if (!is_secboot_active())
		dyn_disable_auth();

#ifdef HSE_SECBOOT_WARNING
	WARN("TWO OR MORE BL AUTHENTICATION KEYS ARE EQUAL.\n");
	WARN("PLEASE USE SEPARATE KEYS IN A PRODUCTION ENVIRONMENT!\n");
#endif
}
#endif

/*
 * We need to provide a dummy implementation for cases when an external DDR
 * Driver is used, since they do not implement this function. The GPRs will be
 * set as part of the external DDR Driver's ddrss_to_io_retention_mode()
 * function, and the ddrss_gpr_to_io_retention_mode() will not be called.
 *
 * WARNING:
 * If the used external DDR Driver version does not support decoupled DDR_GPRs
 * setting, using that external DDR Driver on the SCP flow will lead to an abort
 * if DDR_GPRs are protected from A53 access.
 */
#pragma weak ddrss_gpr_to_io_retention_mode_mmio
void ddrss_gpr_to_io_retention_mode_mmio(void)
{
}

/* Overrides the function from DDR Driver to add SCP flow */
void ddrss_gpr_to_io_retention_mode(void)
{
	int ret;

	if (!is_scp_used()) {
		ddrss_gpr_to_io_retention_mode_mmio();
		return;
	}

	ret = scp_ddrss_gpr_to_io_retention_mode();
	if (ret) {
		ERROR("Failed to set DDRSS GPRs to IO retention mode\n");
		panic();
	}
}

uint32_t deassert_ddr_reset(void)
{
	int ret;

	if (!is_scp_used())
		ret = s32_reset_ddr_periph();
	else
		ret = scp_reset_ddr_periph();

	if (ret < 0)
		ret = -ret;

	return ret;
}

void s32_early_plat_init(void)
{
	uint32_t caiutc;
	int ret;

	ret = s32_plat_config_uart_pinctrl();
	if (ret)
		panic();

	if (!is_scp_used())
		s32_a53_clock_early_setup();
	else
		scp_a53_clock_setup();

	/* Restore (clear) the CAIUTC[IsolEn] bit for the primary cluster, which
	 * we have manually set during early BL2 boot.
	 */
	caiutc = mmio_read_32(S32_NCORE_CAIU0_BASE_ADDR + NCORE_CAIUTC_OFF);
	caiutc &= ~NCORE_CAIUTC_ISOLEN_MASK;
	mmio_write_32(S32_NCORE_CAIU0_BASE_ADDR + NCORE_CAIUTC_OFF, caiutc);

	ncore_init();
	ncore_caiu_online(A53_CLUSTER0_CAIU);

	generic_delay_timer_init();
}

void s32_early_plat_setup(void)
{
	int ret;

	if (s32_el3_mmu_fixup(NULL, 0))
		panic();

	if (!is_scp_used())
		ret = s32_periph_clock_init();
	else
		ret = scp_periph_clock_init();

	if (ret) {
		ERROR("Failed to enable BL2 periph clocks\n");
		panic();
	}
}

void plat_ea_handler(unsigned int ea_reason, uint64_t syndrome, void *cookie,
		void *handle, uint64_t flags)
{
	ERROR("Unhandled External Abort received on 0x%lx at EL3!\n",
	      read_mpidr_el1());
	ERROR(" exception reason=%u syndrome=0x%" PRIx64 "\n", ea_reason, syndrome);

	panic();
}

unsigned int plat_get_syscnt_freq2(void)
{
	return COUNTER_FREQUENCY;
}

struct s32_i2c_driver *s32_add_i2c_module(void *fdt, int fdt_node)
{
	struct s32_i2c_driver *driver;
	struct dt_node_info i2c_info;
	size_t i;
	int ret;

	ret = fdt_node_check_compatible(fdt, fdt_node, "nxp,s32cc-i2c");
	if (ret)
		return NULL;

	for (i = 0; i < i2c_fill_level; i++) {
		if (i2c_drivers[i].fdt_node == fdt_node)
			return &i2c_drivers[i];
	}

	if (i2c_fill_level >= ARRAY_SIZE(i2c_drivers)) {
		INFO("Discovered too many instances of I2C\n");
		return NULL;
	}

	driver = &i2c_drivers[i2c_fill_level];

	dt_fill_device_info(&i2c_info, fdt_node);

	if (i2c_info.base == 0U) {
		INFO("ERROR i2c base\n");
		return NULL;
	}

	driver->fdt_node = fdt_node;
	s32_i2c_get_setup_from_fdt(fdt, fdt_node, &driver->bus);

	i2c_fill_level++;
	return driver;
}

int plat_core_pos_by_mpidr(u_register_t mpidr)
{
	unsigned int cluster_id, cpu_id;

	mpidr &= MPIDR_AFFINITY_MASK;

	if (mpidr & ~(MPIDR_CLUSTER_MASK | MPIDR_CPU_MASK))
		return -1;

	cluster_id = MPIDR_AFFLVL1_VAL(mpidr);
	cpu_id = MPIDR_AFFLVL0_VAL(mpidr);

	if (cluster_id >= PLATFORM_CLUSTER_COUNT ||
	    cpu_id >= PLATFORM_MAX_CPUS_PER_CLUSTER)
		return -1;

	return s32_core_pos_by_mpidr(mpidr);
}

unsigned long get_sdhc_clk_freq(void)
{
	return S32GEN1_SDHC_CLK_FREQ;
}
