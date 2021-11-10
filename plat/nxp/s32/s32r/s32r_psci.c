/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <plat/common/platform.h>

const unsigned char *plat_get_power_domain_tree_desc(void)
{
	__asm__ volatile("b .");
	return NULL;
}

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	__asm__ volatile("b .");

	return 0;
}

