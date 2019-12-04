/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <console_macros.S>
#include "platform_def.h"

.globl platform_mem_init
.globl plat_reset_handler


/* Set the CAIUTC[IsolEn] bit for the primary A53 cluster.
 * This is so cache invalidate operations from the early TF-A boot code
 * won't cause Ncore to crash.
 *
 * Clobber list: x8,x9,x10
 */
func s32g_ncore_isol_cluster0
	movz	x8, #S32G_NCORE_CAIU0_BASE_ADDR_H, lsl #16
	ldr	x9, [x8, #NCORE_CAIUTC_OFF]
	movz	x10, #1
	lsl	x10, x10, #NCORE_CAIUTC_ISOLEN_SHIFT
	orr	x9, x9, x10
	str	x9, [x8, #NCORE_CAIUTC_OFF]

	ret
endfunc s32g_ncore_isol_cluster0


func bl2_sram_init
	/* FIXME I WAS HERE */
	ret
endfunc bl2_sram_init


/* Clobber list: x0,x1,x8,x9,x10,x19 */
func plat_reset_handler
	mov	x19, x30

	/* Ncore quirks */
	bl	s32g_ncore_isol_cluster0

	/* Initialize SRAM, as BootROM did us no favours */
	ldr	x0,=__STACKS_START__
	ldr	x1,=__BL2_END__
	sub	x1, x1, x0
	bl	bl2_sram_init

	mov	x30, x19
	ret
endfunc plat_reset_handler


func platform_mem_init
	ret
endfunc platform_mem_init
