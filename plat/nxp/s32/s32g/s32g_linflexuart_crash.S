/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>

.globl	plat_crash_console_init
.globl	plat_crash_console_putc
.globl	plat_crash_console_flush

#ifndef S32G_CRASH_STACK_SIZE
#define S32G_CRASH_STACK_SIZE	0x400
#endif

/**
 * Use a local buffer as stack for Linflex crash callbacks
 */
.section .data.s32g_plat_crash_stack
	.balign 16
	s32g_plat_crash_stack: .skip S32G_CRASH_STACK_SIZE

.section .data.crash_console_reg_stash
	.align 3
	crash_console_reg_stash: .quad 0, 0, 0, 0, 0, 0

/* Save scratch & link registers and stack pointer */
.macro	save_regs
	mov	x2, sp
	adrp	x1, crash_console_reg_stash
	add	x1, x1, :lo12:crash_console_reg_stash
	stp	x14, x15, [x1]
	stp	x16, x17, [x1, #16]
	stp	x30, x2, [x1, #32]
.endm

/* Restore scratch & link registers and stack pointer */
.macro	restore_regs
	adrp	x1, crash_console_reg_stash
	add	x1, x1, :lo12:crash_console_reg_stash
	ldp	x14, x15, [x1]
	ldp	x16, x17, [x1, #16]
	ldp	x30, x2, [x1, #32]
	mov	sp, x2
.endm

.macro	init_local_stack
	adrp	x1, s32g_plat_crash_stack
	add	x1, x1, :lo12:s32g_plat_crash_stack
	add	x1, x1, #S32G_CRASH_STACK_SIZE
	mov	sp, x1
.endm

func plat_crash_console_init
	mov	x0, #1
	ret
endfunc plat_crash_console_init

func plat_crash_console_putc
	save_regs
	init_local_stack

	bl s32g_plat_crash_console_putc

	restore_regs
	ret
endfunc plat_crash_console_putc

func plat_crash_console_flush
	save_regs
	init_local_stack

	bl s32g_plat_crash_console_flush

	restore_regs
	ret
endfunc plat_crash_console_flush
