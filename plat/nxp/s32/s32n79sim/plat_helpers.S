/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include "platform_def.h"

.globl plat_is_my_cpu_primary
.globl plat_secondary_cold_boot_setup
.globl platform_mem_init
.globl plat_crash_console_init
.globl plat_crash_console_putc
.globl plat_crash_console_flush
.globl plat_my_core_pos

/* unsigned int plat_is_my_cpu_primary(void); */
func plat_is_my_cpu_primary
    ret
endfunc plat_is_my_cpu_primary

/* void plat_secondary_cold_boot_setup(void); */
func plat_secondary_cold_boot_setup
    ret
endfunc plat_secondary_cold_boot_setup

/* void platform_mem_init(void); */
func platform_mem_init
    ret
endfunc platform_mem_init

/* int plat_crash_console_init(void); */
func plat_crash_console_init
    mov_imm x0, PL011_UART_BASE
    mov_imm x1, PL011_UART_CLOCK_HZ
    mov_imm x2, PL011_UART_BAUDRATE
    b console_pl011_core_init
endfunc plat_crash_console_init

/* int plat_crash_console_putc(int); */
func plat_crash_console_putc
    mov_imm	x1, PL011_UART_BASE
    b console_pl011_core_putc
endfunc plat_crash_console_putc

/* void plat_crash_console_flush(void); */
func plat_crash_console_flush
    mov_imm	x0, PL011_UART_BASE
    b console_pl011_core_flush
endfunc plat_crash_console_flush

/* unsigned int plat_my_core_pos(void); */
func plat_my_core_pos
    ret
endfunc plat_my_core_pos
