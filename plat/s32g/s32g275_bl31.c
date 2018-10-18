/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <psci.h>
#include "platform_def.h"

/* TODO should probably be moved to s32g_psci.c */
static uintptr_t warmboot_entry;

static entry_point_info_t bl33_image_ep_info;

/* TODO should probably be moved to s32g_psci.c */
static plat_psci_ops_t s32g_psci_pm_ops = { /* FIXME must implement these */
	.system_reset = NULL,
	.pwr_domain_on = NULL,
	.pwr_domain_on_finish = NULL,
	.pwr_domain_off = NULL,
};

const unsigned char s32g_power_domain_tree_desc[] = {
	PLATFORM_SYSTEM_COUNT,
	PLATFORM_CORE_COUNT,
};

static uint32_t s32g_get_spsr_for_bl33_entry(void)
{
	uint32_t spsr;
	unsigned long el_status, mode;
	unsigned int dbg_current_el;

	/* xDBGx print current EL */
	dbg_current_el = get_current_el();
	printf("Current EL is %u\n", dbg_current_el);

	/* figure out what mode we enter the non-secure world */
	el_status = read_id_aa64pfr0_el1() >> ID_AA64PFR0_EL2_SHIFT;
	el_status &= ID_AA64PFR0_ELX_MASK;

	mode = (el_status) ? MODE_EL2 : MODE_EL1;
	assert(mode == MODE_EL2); /* FIXME debugging only! this must go */

	spsr = SPSR_64(mode, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);

	return spsr;
}

entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	assert(sec_state_is_valid(type));

	return &bl33_image_ep_info;
}

void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
		u_register_t arg2, u_register_t arg3)
{
	static struct console_s32g console;

#if RESET_TO_BL31
	assert((void *)arg0 == NULL); /* from bl2 */
	assert((void *)arg1 == NULL); /* plat params from bl2 */
#endif

	SET_PARAM_HEAD(&bl33_image_ep_info, PARAM_EP, VERSION_1, 0);
	bl33_image_ep_info.pc = S32G_BL33_IMAGE_BASE;
	bl33_image_ep_info.spsr = s32g_get_spsr_for_bl33_entry();
	SET_SECURITY_STATE(bl33_image_ep_info.h.attr, NON_SECURE);

	/* TODO check return */
	console_s32g_register(S32G_UART_BASE, S32G_UART_CLOCK_HZ,
			S32G_UART_BAUDRATE, &console);
}

void bl31_plat_arch_setup(void)
{
	/* TODO implement this */
}

void bl31_platform_setup(void)
{
	/* TODO implement this */
}

int plat_core_pos_by_mpidr(u_register_t mpidr)
{
	unsigned int cpu_id;

	mpidr &= MPIDR_AFFINITY_MASK;

	if (mpidr & ~(MPIDR_CLUSTER_MASK | MPIDR_CPU_MASK))
		return -1;

	cpu_id = MPIDR_AFFLVL0_VAL(mpidr);

	if (cpu_id > PLATFORM_MAX_CPU_PER_CLUSTER)
		return -1;

	return cpu_id;
}

unsigned int plat_get_syscnt_freq2(void)
{
	return COUNTER_FREQUENCY;
}

const unsigned char *plat_get_power_domain_tree_desc(void)
{
	return s32g_power_domain_tree_desc;
}

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	warmboot_entry = sec_entrypoint;
	*psci_ops = &s32g_psci_pm_ops;
	return 0;
}
