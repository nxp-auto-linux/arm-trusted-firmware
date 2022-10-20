/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch_helpers.h>

void cortex_a53_core_pwr_dwn(void);

void __dead2 core_turn_off(void)
{
	cortex_a53_core_pwr_dwn();

	while (true) {
		dsb();
		wfi();
	}
}
