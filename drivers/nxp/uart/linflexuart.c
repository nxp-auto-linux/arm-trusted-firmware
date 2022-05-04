/*
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <drivers/nxp/linflexuart.h>
#include <lib/mmio.h>

#define LDIV_MULTIPLIER		(16)

#define LINFLEX_LINCR1		(0x0)
#define LINCR1_INIT		BIT(0)
#define LINCR1_MME		BIT(4)

#define LINFLEX_LINSR		(0x8)
#define LINSR_LINS_INITMODE	(0x00001000)
#define LINSR_LINS_MASK		(0x0000F000)

#define LINFLEX_UARTCR		(0x10)
#define UARTCR_ROSE		BIT(23)

#define LINFLEX_UARTSR		(0x14)
#define LINFLEX_LINIBRR		(0x28)
#define LINFLEX_LINFBRR		(0x24)
#define LINFLEX_BDRL		(0x38)
#define LINFLEX_UARTPTO		(0x50)

#define UARTCR_UART		BIT(0)
#define UARTCR_WL0		BIT(1)
#define UARTCR_PC0		BIT(3)
#define UARTCR_TXEN		BIT(4)
#define UARTCR_RXEN		BIT(5)
#define UARTCR_PC1		BIT(6)
#define UARTCR_TFBM		BIT(8)
#define UARTCR_RFBM		BIT(9)
#define UARTCR_OSR_MASK		(0xF << 24)
#define UARTCR_OSR(uartcr)	(((uartcr) \
				 & UARTCR_OSR_MASK) >> 24)

#define UARTSR_DTF		BIT(1)

static void linflex_write(uintptr_t base, uintptr_t reg,
			  uint32_t value)
{
	mmio_write_32(base + reg, value);
}

static uint32_t linflex_read(uintptr_t base, uintptr_t reg)
{
	return mmio_read_32(base + reg);
}

static uint32_t get_ldiv_mult(struct console_linflex *cons)
{
	uint32_t mult, cr;
	uintptr_t base = cons->base;

	cr = linflex_read(base, LINFLEX_UARTCR);
	if (cr & UARTCR_ROSE)
		mult = UARTCR_OSR(cr);
	else
		mult = LDIV_MULTIPLIER;

	return mult;
}

static uint32_t get_lin_rate(struct console_linflex *cons)
{
	return cons->clock;
}

static void linflex_set_brg(struct console_linflex *cons)
{
	uint32_t ibr, fbr;
	uintptr_t base = cons->base;
	uint32_t divisr = get_lin_rate(cons);
	uint32_t dividr = (uint32_t)(cons->baud * get_ldiv_mult(cons));

	ibr = (uint32_t)(divisr / dividr);
	fbr = (uint32_t)((divisr % dividr) * 16 / dividr) & 0xF;

	linflex_write(base, LINFLEX_LINIBRR, ibr);
	linflex_write(base, LINFLEX_LINFBRR, fbr);
}

int console_linflex_register(struct console_linflex *cons)
{
	uint32_t ctrl;
	uintptr_t base = cons->base;

	/* Set master mode and init mode */
	ctrl = LINCR1_MME | LINCR1_INIT;
	linflex_write(base, LINFLEX_LINCR1, ctrl);

	/* wait for init mode entry */
	while ((linflex_read(base, LINFLEX_LINSR) & LINSR_LINS_MASK) !=
		LINSR_LINS_INITMODE)
		;

	/* Set UART bit */
	linflex_write(base, LINFLEX_UARTCR, UARTCR_UART);

	linflex_set_brg(cons);

	/* Set preset timeout register value. */
	linflex_write(base, LINFLEX_UARTPTO, 0xf);

	/* 8-bit data, no parity, Tx/Rx enabled, UART mode */
	ctrl = UARTCR_PC1 | UARTCR_RXEN | UARTCR_TXEN | UARTCR_PC0 |
		UARTCR_WL0 | UARTCR_UART | UARTCR_RFBM | UARTCR_TFBM;
	linflex_write(base, LINFLEX_UARTCR, ctrl);

	ctrl = linflex_read(base, LINFLEX_LINCR1);
	ctrl &= ~LINCR1_INIT;
	/* end init mode */
	linflex_write(base, LINFLEX_LINCR1, ctrl);

	console_register(&cons->console);

	return 0;
}

void console_linflex_flush(struct console *console)
{
	return;
}

int console_linflex_putc(int character, struct console *console)
{
	uintptr_t base = console->base;

	if (character == '\n')
		console_linflex_putc('\r', console);

	while (linflex_read(base, LINFLEX_UARTSR) & UARTSR_DTF)
		;

	mmio_write_8(base + LINFLEX_BDRL, character);
	return 0;
}
