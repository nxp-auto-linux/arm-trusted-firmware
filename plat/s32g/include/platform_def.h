/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * TODO: review these definitions. Some of them might not have been
 * correctly and thorougly understood at the time of this writing.
 */

#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

#include <common_def.h>
#include <tbbr_img_def.h>
#include <console.h>

#define S32G_CACHE_WRITEBACK_SHIFT	6
#define CACHE_WRITEBACK_GRANULE		(1 << S32G_CACHE_WRITEBACK_SHIFT)
#define PLAT_PHY_ADDR_SPACE_SIZE        (1ull << 36)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(1ull << 32)

#define PLATFORM_CORE_COUNT		4
#define PLATFORM_SYSTEM_COUNT		1

#define PLAT_NUM_PWR_DOMAINS		(PLATFORM_SYSTEM_COUNT + \
					 PLATFORM_CORE_COUNT)

#define PLAT_MAX_OFF_STATE		2
#define PLAT_MAX_RET_STATE		1
#define PLAT_MAX_PWR_LVL		MPIDR_AFFLVL1
#define PLAT_MAX_PWR_LVL_STATES		2

#define PLAT_PRIMARY_CPU		0x0
#define PLATFORM_MAX_CPU_PER_CLUSTER	4
#define COUNTER_FREQUENCY		0x40000000 /* FXOSC */

/*
 * Platform memory map
 */
#define S32G_XRDC_BASE		0x401A4000
/* SRAM is actually at 0x3400_0000; we are just mirroring it in the
 * Virtual Code RAM
 */
#define S32G_SRAM_BASE		0x38000000
#define S32G_SRAM_SIZE		0x00A00000
#define S32G_DDR0_BASE		0x80000000
/* FIXME this should be a compile-time option; in addition, on S32G we actually
 * have 2 DDR controllers
 */
#define S32G_DDR0_END		0xFFFFFFFF
#define S32G_DDR_SIZE		(S32G_DDR0_END - S32G_DDR0_BASE) /* 2GB */

/* Protected zone at the very top of DDR for our future use */
#define S32G_PMEM_END		(S32G_DDR0_BASE + S32G_DDR_SIZE)
#define S32G_PMEM_LEN		0x00200000	/* 2MB */
#define S32G_PMEM_START		(S32G_PMEM_END - S32G_PMEM_LEN)

/* +----------------------+
 * | Memory layout macros |
 * |----------------------|
 * | FIXME Nice ASCII art,|
 * |      please remove it|
 * v ... ... ... ... ...  v
 */
/* Note: depending on the compiler optimization level, this may or may not be
 * enough to prevent overflowing onto the adjacent SRAM image. Handle with care,
 * wear a helmet and compile with -Os.
 */
#define S32G_BL31_OFF_IN_SRAM		0x00002000
/* U-boot address in SRAM */
#define S32G_BL33_IMAGE_BASE		0x38020000

#define BL31_BASE			(S32G_SRAM_BASE + S32G_BL31_OFF_IN_SRAM)
/* Make sure bl31 does not overlap with u-boot */
#define BL31_LIMIT			(S32G_BL33_IMAGE_BASE - 1)

/* FIXME value randomly chosen; should probably be revisited */
#define PLATFORM_STACK_SIZE		0x4000
/* ^ ... ... ... ... ...  ^
 * |                      |
 * +----------------------+
 */

#define MAX_IO_HANDLES			4
#define MAX_IO_DEVICES			3

/* Debugging options */
#define PLAT_LOG_LEVEL_ASSERT		LOG_LEVEL_VERBOSE

#if defined IMAGE_BL1
/* To use in blX_platform_setup() */
#define FIRMWARE_WELCOME_STR_S32G	"This is S32G BL1\n"
#pragma warning "BL1 image is being built; you should configure it out."
#endif
#if defined IMAGE_BL2 || defined IMAGE_BL2U
#define FIRMWARE_WELCOME_STR_S32G_BL2	"This is S32G BL2\n"
#pragma warning "BL2 image is being built; you should configure it out."
#endif
#if defined IMAGE_BL31
#define FIRMWARE_WELCOME_STR_S32G_BL31	"This is S32G BL31\n"
/* FIXME revisit these */
#define MAX_MMAP_REGIONS		8
#define MAX_XLAT_TABLES			4
#endif
#if defined IMAGE_BL33
#pragma warning "BL33 image is being built; you should configure it out."
#endif

/* Serial console configurations */
#define S32G_LINFLEX0_BASE	0x401C8000
#define S32G_UART_BASE		S32G_LINFLEX0_BASE
#define S32G_UART_BAUDRATE	115200
/* TODO revisit this; for now we'll hard-code the clock/divider settings instead
 * of deriving them
 */
#define S32G_UART_CLOCK_HZ		133333333

/* LINFLEX registers */
#define S32G_LINFLEX_LINCR1		0x0
#define S32G_LINFLEX_LINSR		0x8
#define S32G_LINFLEX_UARTCR		0x10
#define S32G_LINFLEX_UARTSR		0x14
#define S32G_LINFLEX_LINIBRR		0x28
#define S32G_LINFLEX_LINFBRR		0x24
#define S32G_LINFLEX_BDRL		0x38
#define S32G_LINFLEX_UARTPTO		0x50

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
int xrdc_enable(void *xrdc_addr);
#endif /* __ASSEMBLY__ */
#endif /* PLATFORM_DEF_H */
