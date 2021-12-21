/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include "s32_sramc.h"

.globl platform_mem_init
.globl _s32_sram_clr
.globl plat_secondary_cold_boot_setup

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

