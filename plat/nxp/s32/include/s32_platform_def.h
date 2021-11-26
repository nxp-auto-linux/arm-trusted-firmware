/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32_PLATFORM_DEF_H
#define S32_PLATFORM_DEF_H

#include <common_def.h>

#define SIZE_1M                 (0x100000)

#define S32_MPIDR_CPU_CLUSTER_MASK	0xFFF
/* Cluster mask is the most significant 0xF from the CPU_CLUSTER_MASK */
#define S32_MPIDR_CLUSTER_SHIFT	    U(8)
#define S32_PLAT_PRIMARY_CPU		0x0u	/* Cluster 0, cpu 0*/

#define S32_CACHE_WRITEBACK_SHIFT	6
#define CACHE_WRITEBACK_GRANULE		(1 << S32_CACHE_WRITEBACK_SHIFT)
#define PLAT_PHY_ADDR_SPACE_SIZE        (1ull << 36)
/* We'll be doing a 1:1 mapping anyway */
#define PLAT_VIRT_ADDR_SPACE_SIZE	    (1ull << 36)

#define PLATFORM_CLUSTER_COUNT		2
#define PLATFORM_SYSTEM_COUNT		1

#define PLAT_MAX_OFF_STATE		    U(2)
#define PLAT_MAX_RET_STATE		    U(1)
#define PLAT_MAX_PWR_LVL		    MPIDR_AFFLVL2
#define PLAT_MAX_PWR_LVL_STATES		2

/* Generic timer frequency; this goes directly into CNTFRQ_EL0.
 * Its end-value is 5MHz; this is based on the assumption that
 * GPR00[CA53_COUNTER_CLK_DIV_VAL] contains the reset value of 0x7, hence
 * producing a divider value of 8, applied to the FXOSC frequency of 40MHz.
 */
#define COUNTER_FREQUENCY	    0x004C4B40

#define S32_SRAM_BASE		0x34000000
#define S32_SRAM_END		(S32_SRAM_BASE + S32_SRAM_SIZE)

/* Top of the first 2GB bank of physical memory */
#if S32G_EMU == 0
#define S32_DDR0_END		0xffffffff
#else
/* 1GB available */
#define S32_DDR0_END		0xbfffffff
#endif

/* Note: depending on the compiler optimization level, this may or may not be
 * enough to prevent overflowing onto the adjacent SRAM image. Handle with care,
 * wear a helmet and compile with -Os.
 */
#define BOOTROM_ADMA_RSRVD_BASE		(0x343ff000)
#define BL2_LIMIT					(BOOTROM_ADMA_RSRVD_BASE - 1)

/* U-boot addresses in SRAM. BL33_DTB and BL33_ENTRYPOINT must be kept in
 * sync with u-boot's CONFIG_DTB_SRAM_ADDR and CONFIG_SYS_TEXT_BASE.
 */
#define S32_BL33_IMAGE_SIZE	        (5 * SIZE_1M)
/* Leave a gap between BL33 and BL31 to avoid MMU entries merge */
#define BL33_BASE		        (S32_DDR0_END - S32_BL33_IMAGE_SIZE - \
						SIZE_1M + 1)
#define BL33_DTB		        (BL33_BASE + 0x90000)
#define BL33_ENTRYPOINT		    (BL33_BASE + 0xa0000)
#define S32_BL33_IMAGE_BASE	    (BL33_DTB)
#define S32_BL33_LIMIT	        (S32_DDR0_END)

/* Protected zone in DDR - we'll deploy BL31 there. Choose the top of the first
 * 2GB, before the BL33 zone (6 MB), which is reachable by the 32-bit eDMA.
 * This must also be kept in sync with U-Boot, which is expected to alter
 * the Linux device-tree.
 */
#define S32_PMEM_END		(BL33_BASE - 1)
#define S32_PMEM_LEN		(2 * SIZE_1M)	/* conservatively allow 2MB */
#define S32_PMEM_START		(S32_PMEM_END - S32_PMEM_LEN + 1)

/* BL31 location in DDR - physical addresses only, as the MMU is not
 * configured at that point yet
 */
#define BL31_BASE		(S32_PMEM_START)
#define BL31_LIMIT		(S32_PMEM_END)
#define BL31_SIZE		(BL31_LIMIT - BL31_BASE + 1)

/* FIXME value randomly chosen; should probably be revisited */
#define PLATFORM_STACK_SIZE		0x4000

#define MAX_IO_HANDLES			4
#define MAX_IO_DEVICES			3

#define S32_LINFLEX0_BASE	(0x401C8000ul)
#define S32_LINFLEX0_SIZE	(0x4000)
#define S32_LINFLEX1_BASE	(0x401CC000ul)
#define S32_LINFLEX1_SIZE	(0x4000)

#if S32G_EMU == 1
#define S32_UART_BASE		S32_LINFLEX1_BASE
#define S32_UART_SIZE		S32_LINFLEX1_SIZE
#else
#define S32_UART_BASE		S32_LINFLEX0_BASE
#define S32_UART_SIZE		S32_LINFLEX0_SIZE
#endif

#endif /* S32_PLATFORM_H */

