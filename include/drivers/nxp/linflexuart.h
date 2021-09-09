/*
 * Copyright 2019-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef LINFLEXUART_H
#define LINFLEXUART_H

#ifndef __ASSEMBLER__
#include <drivers/console.h>

struct console_linflex {
	console_t console;
	const uintptr_t base;
	const uint32_t  clock;
	const uint32_t  baud;
};

int console_linflex_register(struct console_linflex *console);
int console_linflex_putc(int character, struct console *console);
void console_linflex_flush(struct console *console);
#endif

#endif /* LINFLEXUART_H */
