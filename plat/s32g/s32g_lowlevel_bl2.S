/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <console_macros.S>
#include "platform_def.h"

.globl platform_mem_init
.globl plat_reset_handler
.globl sram_clr
.globl plat_secondary_cold_boot_setup
.globl s32g_ncore_isol_cluster0

/* Clobber list: x0,x1,x7,x8,x9,x10,x11 */
func plat_reset_handler
	mov	x7, x30

	/* Ncore quirks */
	bl	s32g_ncore_isol_cluster0

	/*
	 * Initialize SRAM, as BootROM did us no favours
	 */
	/* Some BL2 sections need to be initialized */
	ldr	x0,=__STACKS_START__
	ldr	x1,=__BL2_END__
	sub	x1, x1, x0
	bl	sram_clr

	mov	x30, x7
	ret
endfunc plat_reset_handler


func platform_mem_init
	ret
endfunc platform_mem_init


func plat_secondary_cold_boot_setup
	ret
endfunc plat_secondary_cold_boot_setup
