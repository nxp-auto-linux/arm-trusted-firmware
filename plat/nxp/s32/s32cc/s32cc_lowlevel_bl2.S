/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <console_macros.S>
#include "platform_def.h"

.globl plat_reset_handler

func plat_reset_handler
	mov	x16, x30

	/* Reset Generic Timers and GPR registers for lockstep */
	bl	reset_registers_for_lockstep

	/* Ncore quirks */
#if S32CC_HAS_NCORE
	bl	s32_ncore_isol_cluster0
#endif

	mov	x30, x16
	ret
endfunc plat_reset_handler
