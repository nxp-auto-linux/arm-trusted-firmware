/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32R47_PLATFORM_DEF_H
#define S32R47_PLATFORM_DEF_H

#include <s32r_platform_def.h>

#define MHZ	UL(1000000)

/* Temporary buffer used by the io block layer. */
#define S32_IO_BUFFER_SIZE		0x100000
#define S32_IO_BUFFER_BASE		(S32_BL32_BASE - S32_IO_BUFFER_SIZE)

#define GPR_BASE_ADDR		(0x440D0000UL)
#define GPR_SIZE			(0x50)
#define	S32_SRAM_BASE		(0x33C00000UL)
#define S32_LINFLEX0_BASE	UL(0x4048C000)
#define S32_LINFLEX0_SIZE	(0x1000)
#define S32_QSPI_BASE		(0x402D0000UL)
#define MSCM_BASE_ADDR		(0x40010000UL)

/* Platform init switches */
#define S32CC_HAS_PINCTRL_ACCESS	0
#define S32CC_HAS_CLOCK_ACCESS		0
#define S32CC_HAS_RESET_ACCESS		0
#define S32CC_HAS_NCORE			0

#define S32R47_SPIN_TABLE_SHIFT 3
#define S32R47_SPIN_TABLE_RSRV	4
#define S32R47_SPIN_TABLE_WAIT	0
#define S32R47_SPIN_TABLE_GO	1

/* The LinFlexD UART0 clock frequency differs from other S32cc SoCs */
#define S32CC_LIN_BAUD_CLK_FREQ		(100 * MHZ)
#undef S32_UART_CLOCK_HZ
#define S32_UART_CLOCK_HZ		S32CC_LIN_BAUD_CLK_FREQ

#undef PLATFORM_MAX_CPUS_PER_CLUSTER
#define PLATFORM_MAX_CPUS_PER_CLUSTER	4

#endif /* S32R47_PLATFORM_H */
