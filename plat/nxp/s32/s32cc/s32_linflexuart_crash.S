/*
 * Copyright 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat_macros.S>
#include <platform_def.h>

.globl	plat_crash_console_init
.globl	plat_crash_console_putc
.globl	plat_crash_console_flush

/* int plat_crash_console_init(void); */
func plat_crash_console_init
	mov_imm	x0, S32_UART_BASE
	mov_imm	x1, S32_UART_CLOCK_HZ
	mov_imm	x2, S32_LINFLEX_BAUDRATE
	b	console_linflex_core_init
endfunc plat_crash_console_init

/* int plat_crash_console_putc(int); */
func plat_crash_console_putc
	mov_imm	x1, S32_UART_BASE
	b	console_linflex_core_putc
	ret
endfunc plat_crash_console_putc

/* void plat_crash_console_flush(void); */
func plat_crash_console_flush
	mov_imm	x0, S32_UART_BASE
	b	console_linflex_core_flush
	ret
endfunc plat_crash_console_flush
