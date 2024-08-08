/*
 * Copyright 2021-2022, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <linflex.h>
#include <platform_def.h>

void console_s32_register(void)
{
	static console_t s32_console = {
		.next = NULL,
		.flags = 0u,
		.flush = console_linflex_flush,
	};
	int ret;

	ret = console_linflex_register(S32_UART_BASE, S32_UART_CLOCK_HZ,
				       S32_LINFLEX_BAUDRATE, &s32_console);
	if (ret == 0) {
		panic();
	}

	console_set_scope(&s32_console,
			  CONSOLE_FLAG_BOOT | CONSOLE_FLAG_CRASH |
			  CONSOLE_FLAG_RUNTIME);
}
