/*
 * Copyright 2019-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <drivers/nxp/s32/rtc/s32g/s32g_rtc.h>
#include "s32g_clocks.h"
#if (ERRATA_S32_050543 == 1)
#include "s32cc_ddr_errata_funcs.h"
#endif
#include "s32cc_linflexuart.h"
#include "s32cc_storage.h"
#include "s32g_mc_rgm.h"
#include "s32g_mc_me.h"
#include "s32cc_bl2_el3.h"
#include "s32g_bl_common.h"
#include "s32g_vr5510.h"
#include "s32cc_sramc.h"
#include <s32cc_scp_scmi.h>
#include <s32cc_scp_utils.h>
#include "s32cc_flexnoc.h"
#if S32CC_EMU == 1
#include "ddrss.h"
#else
#include "ddr_init.h"
#include "ddr_lp.h"
#include <dt-bindings/ddr-errata/s32-ddr-errata.h>
#endif
#if TRUSTED_BOARD_BOOT
#include <drivers/nxp/s32/hse/hse_core.h>
#endif

static bl_mem_params_node_t s32g_bl2_mem_params_descs[7];
REGISTER_BL_IMAGE_DESCS(s32g_bl2_mem_params_descs)
static enum reset_cause reset_cause;

int store_ddr_array_in_ssram(void *array, size_t size, size_t element_size, uintptr_t *current_addr)
{
	if (*current_addr + size * element_size > S32G_SSRAM_LIMIT)
		return -ERANGE;

	memcpy((void *)(*current_addr), array, size * element_size);
	*current_addr += size * element_size;

	return 0;
}

int restore_ddr_array(void **array, size_t size, size_t element_size, uintptr_t *current_addr)
{
	if (*current_addr + size * element_size > S32G_SSRAM_LIMIT)
		return -ERANGE;

	*array = (void *)(*current_addr);
	*current_addr += size * element_size;

	return 0;
}

enum reset_cause get_reset_cause(void)
{
	return reset_cause;
}

static enum reset_cause read_reset_cause(void)
{
	uint32_t mc_rgm_des = mmio_read_32(MC_RGM_DES);
	uint32_t mc_rgm_fes = mmio_read_32(MC_RGM_FES);

	if (mc_rgm_des & DES_F_POR)
		return CAUSE_POR;

	if (mc_rgm_des & DES_F_DR_ANY) {
		if (mmio_read_32(MC_RGM_RDSS) & RDSS_DES_RES)
			return CAUSE_DESTRUCTIVE_RESET_DURING_STANDBY;
		else
			return CAUSE_DESTRUCTIVE_RESET_DURING_RUN;
	}

	if (mc_rgm_fes & FES_F_EXR) {
		if (mmio_read_32(MC_RGM_RDSS) & RDSS_FES_RES)
			return CAUSE_EXTERNAL_RESET_DURING_STANDBY;
		else
			return CAUSE_EXTERNAL_RESET_DURING_RUN;
	}

	if (mc_rgm_fes & FES_F_FR_ANY) {
		if (mmio_read_32(MC_RGM_RDSS) & RDSS_FES_RES)
			return CAUSE_FUNCTIONAL_RESET_DURING_STANDBY;
		else
			return CAUSE_FUNCTIONAL_RESET_DURING_RUN;
	}

	if (mmio_read_32(MC_ME_MODE_STAT) & MODE_STAT_PREV_MODE)
		return CAUSE_WAKEUP_DURING_STANDBY;

	return CAUSE_ERROR;
}

void clear_reset_cause(void)
{
	mmio_setbits_32(MC_RGM_DES, 0);
	mmio_setbits_32(MC_RGM_FES, 0);
	mmio_setbits_32(MC_RGM_RDSS, 0);
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

#pragma weak platform_adjust_noc_settings
int platform_adjust_noc_settings(void)
{
	return 0;
}

static const uintptr_t used_ips_base[] = {
	/* Linflex */
	S32_UART_BASE,
	/* DDR */
	DDRSS_BASE_ADDR,
	DDR_ERRATA_REGION_BASE,
	S32G_SSRAM_BASE,
	GPR_BASE_PAGE_ADDR,
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

/*
 *
 * Since the content of the `configs` structure
 * will be lost during the Suspend to RAM state,
 * the DDR arrays needs to be preserved in SSRAM.
 *
 */
int store_ddr_configs_in_ssram(uintptr_t store_at)
{
	struct s32g_ssram_mailbox *ssram_mailbox;
	uintptr_t current_addr;
	int ret = 0;

	ssram_mailbox = (struct s32g_ssram_mailbox *)store_at;

	ssram_mailbox->ddrc_size = configs[0].ddrc_size;
	ssram_mailbox->phy_size = configs[0].phy_size;
	ssram_mailbox->pie_size = configs[0].pie_size;
	ssram_mailbox->dq_swap_size = configs[0].dq_swap_size;
	ssram_mailbox->ddrc_to_store_size = ddr_lp_configs[0].ddrc_to_store_size;
	ssram_mailbox->csr_settings_size = ddr_lp_configs[0].csr_settings_size;
	ssram_mailbox->csr_to_store_size = ddr_lp_configs[0].csr_to_store_size;

	/*
	 * ------------------------- Current SSRAM Layout
	 * | bl31_warm_entrypoint  |
	 * -------------------------
	 * |        sizes          |
	 * -------------------------
	 * |      csr_settings     |
	 * -------------------------
	 * | ddrc_to_store_settings|
	 * -------------------------
	 *
	 * The `configs` arrays are stored in SSRAM
	 * after the existing content presented above.
	 */
	current_addr = (uintptr_t)&ssram_mailbox->csr_settings[0];
	current_addr += sizeof(ddr_lp_configs[0].csr_settings[0]) *
					ssram_mailbox->csr_settings_size;
	current_addr += sizeof(uint32_t) - (current_addr % sizeof(uint32_t));
	current_addr += sizeof(ddr_lp_configs[0].ddrc_to_store[0]) *
					ssram_mailbox->ddrc_to_store_size;
	if (current_addr > S32G_SSRAM_LIMIT)
		return -ERANGE;

	ret = store_ddr_array_in_ssram(configs[0].ddrc, configs[0].ddrc_size,
				       sizeof(*configs[0].ddrc), &current_addr);
	if (ret != 0)
		panic();

	ret = store_ddr_array_in_ssram(configs[0].dq_swap, configs[0].dq_swap_size,
				       sizeof(*configs[0].dq_swap), &current_addr);
	if (ret != 0)
		panic();

	ret = store_ddr_array_in_ssram(configs[0].phy, configs[0].phy_size,
				       sizeof(*configs[0].phy), &current_addr);
	if (ret != 0)
		panic();

	ret = store_ddr_array_in_ssram(configs[0].pie, configs[0].pie_size,
				       sizeof(*configs[0].pie), &current_addr);
	if (ret != 0)
		panic();

	ret = store_ddr_array_in_ssram(ddr_lp_configs[0].csr_to_store,
				       ddr_lp_configs[0].csr_to_store_size,
				       sizeof(*ddr_lp_configs[0].csr_to_store), &current_addr);
	if (ret != 0)
		panic();

	ret = store_ddr_array_in_ssram(ddr_lp_configs[0].ddrc_to_store,
				       ddr_lp_configs[0].ddrc_to_store_size,
				       sizeof(*ddr_lp_configs[0].ddrc_to_store), &current_addr);
	if (ret != 0)
		panic();

	return 0;

}

/*
 *
 * Restore the DDR arrays from SSRAM back into
 * the configs[] structure. This function
 * ensures all the necessary DDR configuration
 * data is reloaded back into the system when resume.
 *
 */
int restore_ddr_configs_from_ssram(struct s32g_ssram_mailbox *ssram_mb)
{
	uintptr_t current_addr;
	int ret = 0;

	configs[0].ddrc_size = ssram_mb->ddrc_size;
	configs[0].dq_swap_size = ssram_mb->dq_swap_size;
	configs[0].phy_size = ssram_mb->phy_size;
	configs[0].pie_size = ssram_mb->pie_size;
	ddr_lp_configs[0].csr_settings_size = ssram_mb->csr_settings_size;
	ddr_lp_configs[0].csr_to_store_size = ssram_mb->csr_to_store_size;
	ddr_lp_configs[0].ddrc_to_store_size = ssram_mb->ddrc_to_store_size;
	ddr_lp_configs[0].csr_settings = (uint16_t *)&ssram_mb->csr_settings[0];

	current_addr = (uintptr_t)&ssram_mb->csr_settings[0];

	current_addr += sizeof(uint16_t) * ddr_lp_configs[0].csr_settings_size;
	current_addr += sizeof(uint32_t) - (current_addr % sizeof(uint32_t));
	current_addr += sizeof(uint32_t) * ddr_lp_configs[0].ddrc_to_store_size;
	if (current_addr > S32G_SSRAM_LIMIT)
		return -ERANGE;

	ret = restore_ddr_array((void **)&configs[0].ddrc, configs[0].ddrc_size,
				sizeof(*configs[0].ddrc), &current_addr);
	if (ret != 0)
		panic();

	ret = restore_ddr_array((void **)&configs[0].dq_swap, configs[0].dq_swap_size,
				sizeof(*configs[0].dq_swap), &current_addr);
	if (ret != 0)
		panic();

	ret = restore_ddr_array((void **)&configs[0].phy, configs[0].phy_size,
				sizeof(*configs[0].phy), &current_addr);
	if (ret != 0)
		panic();

	ret = restore_ddr_array((void **)&configs[0].pie, configs[0].pie_size,
				sizeof(*configs[0].pie), &current_addr);
	if (ret != 0)
		panic();

	ret = restore_ddr_array((void **)&ddr_lp_configs[0].csr_to_store,
				ddr_lp_configs[0].csr_to_store_size,
				sizeof(*ddr_lp_configs[0].csr_to_store), &current_addr);
	if (ret != 0)
		panic();

	ret = restore_ddr_array((void **)&ddr_lp_configs[0].ddrc_to_store,
				ddr_lp_configs[0].ddrc_to_store_size,
				sizeof(*ddr_lp_configs[0].csr_to_store), &current_addr);
	if (ret != 0)
		panic();

	return 0;

}

static void resume_bl31(struct s32g_ssram_mailbox *ssram_mb)
{
#if S32CC_EMU == 0
	s32g_warm_entrypoint_t resume_entrypoint;
	const struct s32_mmu_filter *mmu_filters;
	size_t n_mmu_filters;
	uintptr_t csr_addr;

	resume_entrypoint = ssram_mb->bl31_warm_entrypoint;
	csr_addr = (uintptr_t)&ssram_mb->csr_settings[0];

	if (s32g_reset_rtc()) {
		ERROR("Failed to reset RTC");
		panic();
	}

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

	if (s32_periph_clock_init()) {
		ERROR("Failed to enable BL2 periph clocks\n");
		panic();
	}

	if (init_and_setup_pmic()) {
		ERROR("Failed to disable VR5510 watchdog\n");
		panic();
	}

	if (platform_adjust_noc_settings()) {
		ERROR("Failed to apply NoC settings\n");
		panic();
	}

	if (restore_ddr_configs_from_ssram(ssram_mb)) {
		ERROR("Failed to restore DDR configs from SSRAM\n");
		panic();
	}

	if (ddrss_to_normal_mode(csr_addr)) {
		ERROR("Failed to transition DDR to normal mode\n");
		panic();
	}

	dsbsy();

	if (s32_el3_mmu_ddr_fixup())
		panic();

	isb();
	dsb();
	disable_mmu_el3();
	resume_entrypoint();
#endif
}

void bl2_el3_early_platform_setup(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	size_t index = 0;
	bl_mem_params_node_t *params = s32g_bl2_mem_params_descs;
	struct s32g_ssram_mailbox *ssram_mb = (void *)BL31SSRAM_MAILBOX;
	size_t params_size = ARRAY_SIZE(s32g_bl2_mem_params_descs);
	int ret = 0;

	if (is_scp_used())
		scp_scmi_init(false);

	if (!is_nvmem_over_scmi_used()) {
		reset_cause = read_reset_cause();
		clear_reset_cause();
	} else {
		ret = scp_get_clear_reset_cause(&reset_cause);
		if (ret)
			ERROR("Failed to get reset cause from SCP\n");
	}

	s32_early_plat_init();

	wait_hse_init();

	if (reset_cause == CAUSE_WAKEUP_DURING_STANDBY) {
		/* Trampoline to bl31_warm_entrypoint */
		resume_bl31(ssram_mb);
		panic();
	}

	s32_early_plat_setup();

	ret |= add_ddr_fw_img_to_mem_params_descs(params, &index, params_size);
	ret |= add_bl31_img_to_mem_params_descs(params, &index, params_size);
	ret |= add_bl32_img_to_mem_params_descs(params, &index, params_size);
	ret |= add_bl32_extra1_img_to_mem_params_descs(params, &index,
							   params_size);
	ret |= add_bl33_img_to_mem_params_descs(params, &index, params_size);
	if (ret)
		panic();

	bl_mem_params_desc_num = index;
}

int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	bl_mem_params_node_t *bl31_params __maybe_unused;

	if (image_id == BL33_IMAGE_ID) {
		return bl2_copy_bl31_dtb();
	}

	if (image_id == BL32_IMAGE_ID) {
		parse_optee(BL32_IMAGE_ID);
	}

	if (image_id == DDR_FW_IMAGE_ID) {
		configure_ddr(DDR_FW_IMAGE_ID);

		if (store_ddr_configs_in_ssram(BL31SSRAM_MAILBOX)) {
			ERROR("Failed to store DDR configs in SSRAM\n");
			panic();
		}

		/* This will also populate CSR section from bl31ssram */
		if (ddr_init()) {
			ERROR("Failed to configure the DDR subsystem\n");
			panic();
		}

		dsbsy();

		if (s32_el3_mmu_ddr_fixup())
			panic();

#if (ERRATA_S32_050543 == 1)
		bl31_params = get_bl_mem_params_node(BL31_IMAGE_ID);
		bl31_params->ep_info.args.arg1 = polling_needed;
#endif

#if TRUSTED_BOARD_BOOT
		hse_use_rmem(ALT_RMEM);
#endif
	}
	return 0;
}

void bl2_el3_plat_arch_setup(void)
{
	console_s32_register();

	if (init_and_setup_pmic())
		panic();

	if (platform_adjust_noc_settings()) {
		ERROR("Failed to apply NoC settings\n");
		panic();
	}

	NOTICE("Reset status: %s\n", get_reset_cause_str());

	s32_sram_clear(S32_IO_BUFFER_BASE,
		       S32_IO_BUFFER_BASE + S32_IO_BUFFER_SIZE);

	s32_ssram_clear();

	clear_swt_faults();

	hse_secboot_setup();
	hse_sram_rmem_clear();

	s32_io_setup();
}
