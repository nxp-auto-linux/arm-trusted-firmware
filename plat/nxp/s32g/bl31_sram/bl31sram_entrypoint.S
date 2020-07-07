/*
 * Copyright (c) 2013-2019, ARM Limited and Contributors. All rights reserved.
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <common/bl_common.h>

.globl	bl31sram_entrypoint

func bl31sram_entrypoint
	/*---------------------------------------------
	 * Save arguments x0 - x3 from BL1 for future
	 * use.
	 * ---------------------------------------------
	 */
	mov	x20, x0
	mov	x21, x1
	mov	x22, x2
	mov	x23, x3

	/* ---------------------------------------------
	 * Zero out NOBITS sections. There are 2 of them:
	 *   - the .bss section;
	 *   - the coherent memory section.
	 * ---------------------------------------------
	 */
	adrp	x0, __BSS_START__
	add	x0, x0, :lo12:__BSS_START__
	adrp	x1, __BSS_END__
	add	x1, x1, :lo12:__BSS_END__
	sub	x1, x1, x0
	bl	zeromem

#if USE_COHERENT_MEM
	adrp	x0, __COHERENT_RAM_START__
	add	x0, x0, :lo12:__COHERENT_RAM_START__
	adrp	x1, __COHERENT_RAM_END_UNALIGNED__
	add	x1, x1, :lo12:__COHERENT_RAM_END_UNALIGNED__
	sub	x1, x1, x0
	bl	zeromem
#endif

	/* --------------------------------------------
	 * Allocate a stack whose memory will be marked
	 * as Normal-IS-WBWA when the MMU is enabled.
	 * There is no risk of reading stale stack
	 * memory after enabling the MMU as only the
	 * primary cpu is running at the moment.
	 * --------------------------------------------
	 */
	bl	plat_set_my_stack

	/* ---------------------------------------------
	 * Initialize the stack protector canary before
	 * any C code is called.
	 * ---------------------------------------------
	 */
#if STACK_PROTECTOR_ENABLED
	bl	update_stack_protector_canary
#endif

	/* ---------------------------------------------
	 * Jump to main function.
	 * ---------------------------------------------
	 */
	bl	bl31sram_main

	/* ---------------------------------------------
	 * Should never reach this point.
	 * ---------------------------------------------
	 */
	no_ret	plat_panic_handler

endfunc bl31sram_entrypoint
