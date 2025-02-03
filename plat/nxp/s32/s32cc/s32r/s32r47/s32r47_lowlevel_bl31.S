/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <console_macros.S>
#include "platform_def.h"

.global plat_secondary_cold_boot_setup
.global s32r47_bl31_spin_loop

func s32r47_bl31_spin_loop
	/* Stack is still invalid and we call functions
	 * which use the stack.
	 */
	bl plat_set_my_stack

#if (S32R47_W_S32R45 == 1) || (S32R47_W_S32G2 == 1)
	/* Make sure that ncore is online. */
	bl	plat_my_core_pos	/* x0: my core index */
	bl	wait_ncore_caiu_online
#endif

loop:
	wfe
	bl plat_my_core_pos /* get param. for is_core_enabled */
	bl is_core_enabled
	cbz x0, loop

	no_ret plat_secondary_cold_boot_setup
endfunc s32r47_bl31_spin_loop

func plat_secondary_cold_boot_setup
	/* Point of no return */
	ldr	x7, =s32_warmboot_entry
	ldr	x7, [x7]
	br	x7
endfunc plat_secondary_cold_boot_setup
