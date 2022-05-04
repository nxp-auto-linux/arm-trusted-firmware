/*
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <drivers/nxp/linflexuart.h>
#include <platform.h>
#include <platform_def.h>

#if S32CC_EMU == 1
#  define S32_UART_BAUDRATE	(7812500)
#else
#  define S32_UART_BAUDRATE	(115200)
#endif
#define S32_UART_CLOCK_HZ	(125000000)

static struct console_linflex console = {
	.base = S32_UART_BASE,
	.clock = S32_UART_CLOCK_HZ,
	.baud = S32_UART_BAUDRATE,
	.console = {
		.putc = console_linflex_putc,
		.flush = console_linflex_flush,
		.flags = CONSOLE_FLAG_BOOT | CONSOLE_FLAG_CRASH,
		.base = S32_UART_BASE,
	},
};

int console_s32_register(void)
{
	return console_linflex_register(&console);
}

int s32_plat_crash_console_putc(int c)
{
	return console_linflex_putc(c, &console.console);
}

void s32_plat_crash_console_flush(void)
{
	return console_linflex_flush(&console.console);
}
