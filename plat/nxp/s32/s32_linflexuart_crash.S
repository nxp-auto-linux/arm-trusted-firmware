/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <plat_macros.S>

.globl	plat_crash_console_init
.globl	plat_crash_console_putc
.globl	plat_crash_console_flush

func plat_crash_console_init
	mov	x0, #1
	ret
endfunc plat_crash_console_init

func plat_crash_console_putc
	s32_save_regs
	s32_init_local_stack

	bl s32_plat_crash_console_putc

	s32_check_stack_guard
	s32_restore_regs
	ret
endfunc plat_crash_console_putc

func plat_crash_console_flush
	s32_save_regs
	s32_init_local_stack

	bl s32_plat_crash_console_flush

	s32_check_stack_guard
	s32_restore_regs
	ret
endfunc plat_crash_console_flush
