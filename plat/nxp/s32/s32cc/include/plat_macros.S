/*
 * Copyright 2019-2021, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLAT_MACROS_S__
#define __PLAT_MACROS_S__

#include <asm_macros.S>

#ifndef S32_CRASH_STACK_SIZE
#define S32_CRASH_STACK_SIZE	0x400
#endif

#define S32_STACK_GUARD			0xDEAD

.extern s32_plat_data_stack
.extern s32_crash_reg_stash

/* ---------------------------------------------
 * The below required platform porting macro
 * prints out relevant GIC and CCI registers
 * whenever an unhandled exception is taken in
 * BL31.
 * Clobbers: x0 - x10, x16, x17, sp
 * ---------------------------------------------
 */
.macro plat_crash_print_regs
.endm

/**
 * Save intra-procedure-call scratch, frame pointer and
 * link registers (x16, x17, x29, x30)
 *
 * Clobbers: reg1, reg2
 */
.macro	s32_save_regs reg1=x9, reg2=x10
	mov	\reg1, sp
	adrp	\reg2, s32_crash_reg_stash
	add	\reg2, \reg2, :lo12:s32_crash_reg_stash
	stp	x16, x17, [\reg2]
	stp	x29, x30, [\reg2, #16]
.endm

/**
 * Restore intra-procedure-call scratch, frame pointer
 * and link registers (x16, x17, x29, x30)
 *
 * Clobbers: reg1, reg2
 */
.macro	s32_restore_regs reg1=x9, reg2=x10
	adrp	\reg2, s32_crash_reg_stash
	add	\reg2, \reg2, :lo12:s32_crash_reg_stash
	ldp	x16, x17, [\reg2]
	ldp	x29, x30, [\reg2, #16]
	mov	sp, \reg1
.endm

.macro	s32_init_local_stack
	mov	w10, #S32_STACK_GUARD
	adrp	x9, s32_plat_data_stack
	add	x9, x9, :lo12:s32_plat_data_stack
	/* Color the end of the stack */
	str	w10, [x9]
	add	x9, x9, #S32_CRASH_STACK_SIZE
	mov	sp, x9
.endm

.macro s32_check_stack_guard
	mov	w10, #S32_STACK_GUARD
	adrp	x9, s32_plat_data_stack
	add	x9, x9, :lo12:s32_plat_data_stack
	ldrh	w9, [x9]
	cmp	w9, w10
	b.ne	plat_panic_handler
.endm

#endif /* __PLAT_MACROS_S__ */
