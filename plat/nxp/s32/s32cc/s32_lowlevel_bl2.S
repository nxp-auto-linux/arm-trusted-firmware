/*
 * Copyright 2021, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <console_macros.S>
#include "platform_def.h"
#include "s32cc_sramc.h"

.globl platform_mem_init
.globl plat_reset_handler
.globl _s32_sram_clr
.globl plat_secondary_cold_boot_setup
.globl s32_ncore_isol_cluster0
.globl reset_registers_for_lockstep

/* Clobber list: x0,x1,x16 */
func plat_reset_handler
	mov	x16, x30

	/* Reset Generic Timers and GPR registers for lockstep */
	bl	reset_registers_for_lockstep

	/* Ncore quirks */
	bl	s32_ncore_isol_cluster0

	mov	x30, x16
	ret
endfunc plat_reset_handler

/* Clobber list: x0,x1,x16 */
func platform_mem_init
	mov	x16, x30
	/*
	 * Initialize SRAM, as BootROM did us no favours
	 */
	/* Some BL2 sections need to be initialized */
	ldr	x0,=__STACKS_START__
	ldr	x1,=__BL2_END__

	bl	_s32_sram_clr
	mov	x30, x16

	ret
endfunc platform_mem_init

func plat_secondary_cold_boot_setup
	ret
endfunc plat_secondary_cold_boot_setup

