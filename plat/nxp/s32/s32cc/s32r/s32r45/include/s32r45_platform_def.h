/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32R45_PLATFORM_DEF_H
#define S32R45_PLATFORM_DEF_H

#include <s32r_platform_def.h>

/* Temporary buffer used by the io block layer. */
#define S32_IO_BUFFER_BASE		(BL2_LIMIT - S32_IO_BUFFER_SIZE + 1)

#define GPR_BASE_ADDR			(0x4007C400UL)
#define GPR_SIZE				(0xb4)

#define S32_SRAM_BASE			(0x34000000UL)

#define S32_LINFLEX0_BASE		(0x401C8000UL)
#define S32_LINFLEX0_SIZE		(0x4000)

#define S32_QSPI_BASE			(0x40134000UL)

#define MSCM_BASE_ADDR			(0x40198000UL)

/* Platform init switches */
#define S32CC_HAS_PINCTRL_ACCESS	1
#define S32CC_HAS_CLOCK_ACCESS		1
#define S32CC_HAS_RESET_ACCESS		1
#define S32CC_HAS_NCORE				1

#endif /* S32R45_PLATFORM_H */
