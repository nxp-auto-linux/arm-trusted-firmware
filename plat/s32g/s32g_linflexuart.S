/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <console_macros.S>
#include "s32g_linflexuart.h"

.globl console_s32g_register
.globl console_s32g_putc
.globl console_s32g_flush

.globl plat_crash_console_init
.globl plat_crash_console_flush
.globl plat_crash_console_putc

func plat_crash_console_init
endfunc plat_crash_console_init

func plat_crash_console_flush
endfunc plat_crash_console_flush

func plat_crash_console_putc
endfunc plat_crash_console_putc

/* int console_s32g_register(uintptr_t base,
 *			     uint32_t clk,
 *			     uint32_t baud,
 *			     struct console_s32g *console)
 * Hard-coded configuration: 8 data bits, no parity, 1 stop bit, no start bits
 *
 * In: x0 - UART register base address
 *     w1 - UART clock in Hz
 *     w2 - baud rate
 *     x3 - pointer to empty struct console_s32g
 * Out: x0 = 1 on success, 0 on error
 * Clobber list: x0,x1,x2,x3,x6,x7,x16,x17
 */
func console_s32g_register
	mov	x7, x30		/* back up return address */

	/* Set master mode and init mode */
	movz	w16, #0x10	/* LINCR1_MME */
	movz	w17, #0x1	/* LINCR1_INIT */
	orr	w16, w16, w17
	str	w16, [x0, #S32G_LINFLEX_LINCR1]

	/* wait for init mode entry */
	movz	w16, #0x1000	/* LINSR_LINS_INITMODE */
	movz	w17, #0xF000	/* LINSR_LINS_MASK */
wait_init_mode:
	ldr	w6, [x0, #S32G_LINFLEX_LINSR]
	and	w6, w6, w17
	eor	w6, w6, w16
	cbnz	w6, wait_init_mode

	/* Set UART bit */
	movz	w16, #0x1	/* UARTCR_UART */
	str	w16, [x0, #S32G_LINFLEX_UARTCR]

	/* hard-coded linflexd_serial_setbrg() result,
	 * working with UARTCR[ROSE]=0
	 * FIXME: real implementation needed */
	movz	w16, #72		/* ibr */
	str	w16, [x0, #S32G_LINFLEX_LINIBRR]
	movz	w16, #5		/* fbr */
	str	w16, [x0, #S32G_LINFLEX_LINFBRR]

	/* Set preset timeout register value. */
	movz	w16, #0xf
	str	w16, [x0, #S32G_LINFLEX_UARTPTO]

	/* 8-bit data, no parity, Tx/Rx enabled, UART mode */
	movz	w16, #0x40	/* UARTCR_PC1 */
	movz	w17, #0x20	/* UARTCR_RXEN */
	orr	w16, w16, w17
	movz	w17, #0x10	/* UARTCR_TXEN */
	orr	w16, w16, w17
	movz	w17, #0x8	/* UARTCR_PC0 */
	orr	w16, w16, w17
	movz	w17, #0x2	/* UARTCR_WL0 */
	orr	w16, w16, w17
	movz    w17, #0x1       /* UARTCR_UART */
	orr     w16, w16, w17
	movz	w17, #0x200	/* UARTCR_RFBM */
	orr	w16, w16, w17
	movz	w17, #0x100	/* UARTCR_TFBM */
	orr	w16, w16, w17
	str	w16, [x0, #S32G_LINFLEX_UARTCR]

	ldr	w16, [x0, #S32G_LINFLEX_LINCR1]
	movz	w17, #0x1	/* LINCR1_INIT */
	orn	w16, w17, w16
	movz	w17, 0xFFFF
	eor	w16, w16, w17
	str	w16, [x0, #S32G_LINFLEX_LINCR1]

	/* prepare to finish console registration */
	mov	x0, x3
	mov	x30, x7
	finish_console_register s32g

	movz	w0, 1
	ret
endfunc console_s32g_register

/* In:  w0 - character to be printed
 *      x1 - pointer to the console_s32g structure (FIXME: currently ignored)
 * Out: w0 - printed character on success, < 0 on error
 * Clobber list: x0,x1,x2,x5,x6
 */
func console_s32g_putc
	/* FIXME: Do not hardcode the UART base addr; instead, pass it via the
	          console struct */
	/* S32G_UART_BASE */
	movz	x6, #0x401C, lsl #16
	movk	x6, #0x8000

	/* if c == '\n', also put a '\r' beforehand */
	movz	w2, #0
	cmp	w0, #0xA
	b.ne	putc_this

	mov	w2, w0
	movz	w0, #0xD
putc_this:
tx_fifo_full:
	ldr	w5, [x6, #S32G_LINFLEX_UARTSR]
	tbnz	w5, #1, tx_fifo_full	/* UARTSR_DTFTFF */
	strb	w0, [x6, #S32G_LINFLEX_BDRL]

	cbz	w2, done
	mov	w0, w2
	movz	w2, #0
	b	putc_this
done:
	ret
endfunc console_s32g_putc

func console_s32g_flush
	ret
endfunc console_s32g_flush
