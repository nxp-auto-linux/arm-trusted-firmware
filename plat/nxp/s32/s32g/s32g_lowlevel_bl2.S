/*
 * Copyright 2019-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <console_macros.S>
#include "platform_def.h"

.globl plat_reset_handler
.globl s32g_ncore_isol_cluster0
.globl reset_registers_for_lockstep

/* Clobber list: x0,x1,x16 */
func plat_reset_handler
	mov	x16, x30

	/* Reset Generic Timers and GPR registers for lockstep */
	bl	reset_registers_for_lockstep

	/* Ncore quirks */
	bl	s32g_ncore_isol_cluster0

	mov	x30, x16
	ret
endfunc plat_reset_handler

