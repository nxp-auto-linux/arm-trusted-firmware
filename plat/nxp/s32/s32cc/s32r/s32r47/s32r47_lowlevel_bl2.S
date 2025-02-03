/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <console_macros.S>
#include "platform_def.h"

.globl plat_reset_handler
.globl plat_secondary_cold_boot_setup

func plat_reset_handler
	mov	x16, x30

	/* Reset Generic Timers and GPR registers.
	 * Lockstep is not present on S32R47 but we
	 * are reusing the same function.
	 */
	bl	reset_registers_for_lockstep

#if (S32R47_W_S32R45 == 1) || (S32R47_W_S32G2 == 1)
	/* Ncore quirks */
	bl plat_my_core_pos
	cbnz x0, skip_ncore
	bl	s32_ncore_isol_cluster0
	bl	s32_ncore_isol_cluster1

skip_ncore:
#endif
	mov	x30, x16
	ret
endfunc plat_reset_handler


/* Clobber list: x0, x1, x2 */
func plat_secondary_cold_boot_setup
	bl plat_my_core_pos
	mov x1, x0

	/* No need for cache invalidation since
	 * the page containing the spin table
	 * will be marked as non-cacheable.
	 *
	 * Since this code runs early during the
	 * boot process it is safe to initialize
	 * the spin table for the current core.
	 * The core will be release later by
	 * Linux.
	 */
	ldr x0, =S32R47_BL2_SPIN_TABLE_ADDR
	cbnz x0, normal_flow
	no_ret plat_panic_handler

normal_flow:
	mov x2, S32R47_SPIN_TABLE_WAIT
	/* Initialize the lock. */
	strb w2, [x0, x1]
	/* Initialize the jump address with an invalid one. */
	mov x2, #0
	add x0, x0, #(PLATFORM_CORE_COUNT + S32R47_SPIN_TABLE_RSRV)
	str x2, [x0, x1, lsl #S32R47_SPIN_TABLE_SHIFT]
	sub x0, x0, #(PLATFORM_CORE_COUNT + S32R47_SPIN_TABLE_RSRV)

spin_table_loop:
	wfe
	ldrb w2, [x0, x1]
	cmp w2, S32R47_SPIN_TABLE_GO
	bne spin_table_loop

	/* Core 0 loaded BL31, must jump there. */
	add x0, x0, #(PLATFORM_CORE_COUNT + S32R47_SPIN_TABLE_RSRV)
	ldr x2, [x0, x1, lsl #S32R47_SPIN_TABLE_SHIFT]
	cbz x2, error
	br x2
error:
	/* Shouldn't get here */
	no_ret plat_panic_handler
endfunc plat_secondary_cold_boot_setup

