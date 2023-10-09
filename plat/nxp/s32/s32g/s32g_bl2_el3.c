/*
 * Copyright 2019-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include "s32g_clocks.h"
#if (ERRATA_S32_050543 == 1)
#include "s32_ddr_errata_funcs.h"
#endif
#include "s32_linflexuart.h"
#include "s32_storage.h"
#include "s32g_mc_rgm.h"
#include "s32g_mc_me.h"
#include "s32_bl2_el3.h"
#include "s32g_bl_common.h"
#include "s32g_vr5510.h"
#include "s32_pinctrl.h"
#include "s32_sramc.h"
#include <s32_scp_scmi.h>
#include <s32_scp_utils.h>
#if S32CC_EMU == 1
#include "ddrss.h"
#else
#include "ddr_init.h"
#include "ddr_lp.h"
#include <dt-bindings/ddr-errata/s32-ddr-errata.h>
#endif

static bl_mem_params_node_t s32g_bl2_mem_params_descs[6];
REGISTER_BL_IMAGE_DESCS(s32g_bl2_mem_params_descs)

static enum reset_cause get_reset_cause(void)
{
	uint32_t mc_rgm_des = mmio_read_32(MC_RGM_DES);

	if (mc_rgm_des & DES_F_POR)
		return CAUSE_POR;

	if (mc_rgm_des & DES_F_DR_ANY) {
		if (mmio_read_32(MC_RGM_RDSS) & RDSS_DES_RES)
			return CAUSE_DESTRUCTIVE_RESET_DURING_STANDBY;
		else
			return CAUSE_DESTRUCTIVE_RESET_DURING_RUN;
	}

	if (mmio_read_32(MC_RGM_FES) & FES_F_FR_ANY) {
		if (mmio_read_32(MC_RGM_RDSS) & RDSS_FES_RES)
			return CAUSE_FUNCTIONAL_RESET_DURING_STANDBY;
		else
			return CAUSE_FUNCTIONAL_RESET_DURING_RUN;
	}

	if (mmio_read_32(MC_ME_MODE_STAT) & MODE_STAT_PREV_MODE)
		return CAUSE_WAKEUP_DURING_STANDBY;

	return CAUSE_ERROR;
}

static int init_and_setup_pmic(void)
{
	int ret = 0;

	if (is_scp_used())
		return 0;

	dt_init_ocotp();
	dt_init_pmic();

#if S32CC_EMU == 0
	ret = pmic_setup();
	if (ret)
		ERROR("Failed to disable VR5510 watchdog\n");
#endif

	return ret;
}

static __unused void reset_rtc(void)
{
	uint32_t rtc = S32G_RTC_BASE;
	uint32_t rtcs;

	mmio_write_32(rtc + RTC_APIVAL_OFFSET, 0x0);
	mmio_write_32(rtc + RTC_RTCVAL_OFFSET, 0x0);

	mmio_write_32(rtc + RTC_RTCC_OFFSET, 0x0);

	rtcs = mmio_read_32(rtc + RTC_RTCS_OFFSET);
	mmio_write_32(rtc + RTC_RTCS_OFFSET, rtcs);
}

static const uintptr_t used_ips_base[] = {
	/* Linflex */
	S32_UART_BASE,
	/* Pinmuxing */
	SIUL2_0_BASE_ADDR,
	SIUL2_1_BASE_ADDR,
	/* PMIC */
	I2C4_BASE_ADDR,
	OCOTP_BASE_ADDR,
	/* DDR */
	DDRSS_BASE_ADDR,
	DDR_ERRATA_REGION_BASE,
	S32G_SSRAM_BASE,
	GPR_BASE_PAGE_ADDR,
};

static const uintptr_t clock_ips[] = {
	MC_CGM5_BASE_ADDR,
	S32_MC_RGM_BASE_ADDR,
	S32_MC_ME_BASE_ADDR,
	DRAM_PLL_BASE_ADDR,
	S32_FXOSC_BASE_ADDR,
};

static const uintptr_t scp_used_ips[] = {
	S32_SCP_SCMI_MEM,
	MSCM_BASE_ADDR,
#if defined(STM6_BASE_ADDR)
	STM6_BASE_ADDR,
#endif
};

static const struct s32_mmu_filter non_scp_filters[] = {
	{
		.base_addrs = used_ips_base,
		.n_base_addrs = ARRAY_SIZE(used_ips_base),
	},
	{
		.base_addrs = clock_ips,
		.n_base_addrs = ARRAY_SIZE(clock_ips),
	},
};

static const struct s32_mmu_filter scp_filters[] = {
	{
		.base_addrs = used_ips_base,
		.n_base_addrs = ARRAY_SIZE(used_ips_base),
	},
	{
		.base_addrs = scp_used_ips,
		.n_base_addrs = ARRAY_SIZE(scp_used_ips),
	},
};

static void resume_bl31(struct s32g_ssram_mailbox *ssram_mb)
{
#if S32CC_EMU == 0
	s32g_warm_entrypoint_t resume_entrypoint;
	const struct s32_mmu_filter *mmu_filters;
	size_t n_mmu_filters;
	uintptr_t csr_addr;

	resume_entrypoint = ssram_mb->bl31_warm_entrypoint;
	csr_addr = (uintptr_t)&ssram_mb->csr_settings[0];

	reset_rtc();

	if (is_scp_used()) {
		mmu_filters = &scp_filters[0];
		n_mmu_filters = ARRAY_SIZE(scp_filters);
	} else {
		mmu_filters = &non_scp_filters[0];
		n_mmu_filters = ARRAY_SIZE(non_scp_filters);
	}

	/*
	 * Configure MMU & caches for a short period of time needed to boost
	 * the DTB parsing and DDR reconfig.
	 */
	if (s32_el3_mmu_fixup(mmu_filters, n_mmu_filters)) {
		ERROR("Failed to configure MMU");
		panic();
	}

	if (init_and_setup_pmic()) {
		ERROR("Failed to disable VR5510 watchdog\n");
		panic();
	}

	if (ddrss_to_normal_mode(csr_addr)) {
		ERROR("Failed to transition DDR to normal mode\n");
		panic();
	}

#if (ERRATA_S32_050543 == 1)
	ddr_errata_update_flag(polling_needed);
#endif

	isb();
	dsb();
	disable_mmu_el3();
	resume_entrypoint();
#endif
}

void bl2_el3_early_platform_setup(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	enum reset_cause reset_cause;
	size_t index = 0;
	bl_mem_params_node_t *params = s32g_bl2_mem_params_descs;
	struct s32g_ssram_mailbox *ssram_mb = (void *)BL31SSRAM_MAILBOX;
	size_t params_size = ARRAY_SIZE(s32g_bl2_mem_params_descs);
	int ret = 0;

	if (is_scp_used())
		scp_scmi_init(false);

	if (!is_nvmem_over_scmi_used()) {
		reset_cause = get_reset_cause();
		clear_reset_cause();
	} else {
		ret = scp_get_clear_reset_cause(&reset_cause);
		if (ret)
			ERROR("Failed to get reset cause from SCP\n");
	}

	s32_early_plat_init();
	console_s32_register();

#ifdef HSE_SUPPORT
	/* if HSE FW is present, write HSE_CONFIG_PERIPH_DONE
	 * to signal clock config is done */
	mmio_write_32(HSE_GCR, HSE_PERIPH_CONFIG_DONE);
#endif

	if (reset_cause == CAUSE_WAKEUP_DURING_STANDBY) {
		/* Trampoline to bl31_warm_entrypoint */
		resume_bl31(ssram_mb);
		panic();
	}

#ifdef HSE_SUPPORT
	/* if HSE FW is present, wait until HSE FW signals
	 * that init is complete - HSE_STATUS_INIT_OK */
	while ((mmio_read_32(HSE_FSR) & HSE_STATUS_INIT_OK) == 0)
		;
#endif

	NOTICE("Reset status: %s\n", get_reset_cause_str(reset_cause));

	s32_plat_config_sdhc_pinctrl();
	s32_io_setup();

	ret |= add_bl31_img_to_mem_params_descs(params, &index, params_size);
	ret |= add_bl32_img_to_mem_params_descs(params, &index, params_size);
	ret |= add_bl32_extra1_img_to_mem_params_descs(params, &index,
						       params_size);
	ret |= add_bl33_img_to_mem_params_descs(params, &index, params_size);
	ret |= add_invalid_img_to_mem_params_descs(params, &index,
						   params_size);
	if (ret)
		panic();

	bl_mem_params_desc_num = index;
}

void bl2_el3_plat_arch_setup(void)
{
	if (s32_el3_mmu_fixup(NULL, 0))
		panic();

	if (init_and_setup_pmic())
		panic();

	s32_sram_clear(S32_BL33_IMAGE_BASE, get_bl2_dtb_base());

	s32_ssram_clear();

	clear_swt_faults();

	/* This will also populate CSR section from bl31ssram */
	if (ddr_init()) {
		ERROR("Failed to configure the DDR subsystem\n");
		panic();
	}

#if (ERRATA_S32_050543 == 1)
	ddr_errata_update_flag(polling_needed);
#endif
}
