/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <stdint.h>

#include <s32_derating_erratum.h>

static uint8_t ddr_erratum_polling_flag;

void s32_set_ddr_erratum_polling_flag(uint8_t polling_value)
{
	ddr_erratum_polling_flag = polling_value;
}
