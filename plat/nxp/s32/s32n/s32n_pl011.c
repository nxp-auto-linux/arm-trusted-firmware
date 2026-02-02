/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <drivers/arm/pl011.h>
#include <platform_def.h>

void console_s32n_register(uint32_t rate)
{
	static console_t s32_console = {
		.next = NULL,
		.flags = 0u,
	};
	int ret;

	ret = console_pl011_register(PL011_UART_BASE,
			rate, PL011_UART_BAUDRATE, &s32_console);
	if (ret == 0)
		panic();

	console_set_scope(&s32_console,
		CONSOLE_FLAG_BOOT | CONSOLE_FLAG_CRASH |
		CONSOLE_FLAG_RUNTIME);
}
