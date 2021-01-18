/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <drivers/nxp/linflexuart.h>
#include <platform.h>
#include <platform_def.h>

#define S32G_UART_BAUDRATE	(115200)
#define S32G_UART_CLOCK_HZ	(125000000)

static struct console_linflex console = {
	.base = S32G_UART_BASE,
	.clock = S32G_UART_CLOCK_HZ,
	.baud = S32G_UART_BAUDRATE,
	.console = {
		.putc = console_linflex_putc,
		.flush = console_linflex_flush,
		.flags = CONSOLE_FLAG_BOOT | CONSOLE_FLAG_CRASH,
		.base = S32G_UART_BASE,
	},
};

int console_s32g_register(void)
{
	return console_linflex_register(&console);
}

int plat_crash_console_init(void)
{
	return 0;
}

int plat_crash_console_flush(void)
{
	return console_linflex_flush(&console.console);
}

int plat_crash_console_putc(int c)
{
	return console_linflex_putc(c, &console.console);
}
