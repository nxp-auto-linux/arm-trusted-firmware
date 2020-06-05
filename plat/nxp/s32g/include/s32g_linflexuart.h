/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32G_LINFLEXUART_H
#define S32G_LINFLEXUART_H

#define S32G_LINFLEX0_BASE	(0x401C8000ul)
#define S32G_LINFLEX0_SIZE	(0x4000)
#define S32G_UART_BASE		S32G_LINFLEX0_BASE
#define S32G_UART_SIZE		S32G_LINFLEX0_SIZE
#define S32G_UART_BAUDRATE	(115200)
#define S32G_UART_CLOCK_HZ	(133333333)

#define S32G_LINFLEX_LINCR1	(0x0)
#define S32G_LINFLEX_LINSR	(0x8)
#define S32G_LINFLEX_UARTCR	(0x10)
#define S32G_LINFLEX_UARTSR	(0x14)
#define S32G_LINFLEX_LINIBRR	(0x28)
#define S32G_LINFLEX_LINFBRR	(0x24)
#define S32G_LINFLEX_BDRL	(0x38)
#define S32G_LINFLEX_UARTPTO	(0x50)

#ifndef __ASSEMBLY__
struct console_s32g {
	console_t console;
	uint32_t  size;
	uintptr_t base;
	uint32_t  clock;
	uint32_t  baud;
};

int console_s32g_register(uintptr_t baseaddr, uint32_t clock, uint32_t baud,
			  struct console_s32g *console);
int console_s32g_putc(int c, struct console_s32g *console);
int console_s32g_flush(struct console_s32g *console);
#endif

#endif /* S32G_LINFLEXUART_H */
