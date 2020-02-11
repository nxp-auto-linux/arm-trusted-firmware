/*
 * Copyright 2019-2020 NXP
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

#define SIZE_1M		(1024 * 1024)

/* MPIDR_EL1 for the four A53 cores is as follows:
 *	A53_0_cpu0:	0x8000_0000
 *	A53_0_cpu1:	0x8000_0001
 *	A53_1_cpu0:	0x8000_0100
 *	A53_1_cpu1:	0x8000_0101
 */
#define S32G_MPIDR_CPU_MASK		0xFF
#define S32G_MPIDR_CPU_CLUSTER_MASK	0xFFF
/* Cluster mask is the most significant 0xF from the CPU_CLUSTER_MASK */
#define S32G_MPIDR_CLUSTER_SHIFT	U(8)
#define S32G_PLAT_PRIMARY_CPU		0x0u	/* Cluster 0, cpu 0*/

#define S32G_NCORE_CAIU0_BASE_ADDR	0x50400000
#define S32G_NCORE_CAIU0_BASE_ADDR_H	(S32G_NCORE_CAIU0_BASE_ADDR >> 16)
#define NCORE_CAIUTC_OFF		0x0
#define NCORE_CAIUTC_ISOLEN_SHIFT	1
#define NCORE_CAIUTC_ISOLEN_MASK	BIT(NCORE_CAIUTC_ISOLEN_SHIFT)

#define S32G_CACHE_WRITEBACK_SHIFT	6
#define CACHE_WRITEBACK_GRANULE		(1 << S32G_CACHE_WRITEBACK_SHIFT)
#define PLAT_PHY_ADDR_SPACE_SIZE        (1ull << 36)
/* We'll be doing a 1:1 mapping anyway */
#define PLAT_VIRT_ADDR_SPACE_SIZE	(1ull << 36)

#define PLATFORM_CORE_COUNT		4
#define PLATFORM_CLUSTER_COUNT		2
#define PLATFORM_SYSTEM_COUNT		1
/* FIXME I'm not sure this is technically correct. We do NOT have
 * cluster-level power management operations, only core and system.
 */
#define PLAT_NUM_PWR_DOMAINS		(PLATFORM_SYSTEM_COUNT + \
					 PLATFORM_CLUSTER_COUNT + \
					 PLATFORM_CORE_COUNT)

#define PLAT_MAX_OFF_STATE		U(2)
#define PLAT_MAX_RET_STATE		U(1)
#define PLAT_MAX_PWR_LVL		MPIDR_AFFLVL2
#define PLAT_MAX_PWR_LVL_STATES		2

#define PLAT_PRIMARY_CPU		0x0
/* Generic timer frequency; this goes directly into CNTFRQ_EL0.
 * Its end-value is 5MHz; this is based on the assumption that
 * GPR00[CA53_COUNTER_CLK_DIV_VAL] contains the reset value of 0x7, hence
 * producing a divider value of 8, applied to the FXOSC frequency of 40MHz.
 */
#define COUNTER_FREQUENCY	0x004C4B40

#define SIUL2_0_BASE_ADDR	0x4009C000UL
#define SIUL2_1_BASE_ADDR	0x44010000UL

#define GPR_BASE_ADDR		0x4007C400UL
#define GPR06_OFF		0x18
#define GPR09_OFF		0x24
#define CA53_0_0_RVBARADDR_39_32_OFF	(0)
#define CA53_0_0_RVBARADDR_39_32_MASK	(0xFFUL)
#define CA53_0_1_RVBARADDR_39_32_OFF	(8)
#define CA53_0_1_RVBARADDR_39_32_MASK	(0xFFUL)
#define CA53_1_0_RVBARADDR_39_32_OFF	(16)
#define CA53_1_0_RVBARADDR_39_32_MASK	(0xFFUL)
#define CA53_1_1_RVBARADDR_39_32_OFF	(24)
#define CA53_1_1_RVBARADDR_39_32_MASK	(0xFFUL)

#define S32G_XRDC_0_PAC_0_BASE_ADDR	0x40000000ULL
#define S32G_XRDC_0_PAC_0_SIZE		SIZE_1M

/* GIC (re)definitions */
#define S32G275_GIC_BASE	0x50800000
#define PLAT_GICD_BASE		S32G275_GIC_BASE
#define PLAT_GICR_BASE		(S32G275_GIC_BASE + 0x80000)
#define S32G275_GIC_SIZE	0x100000
/* SGI to use for kicking the secondary cores out of wfi */
#define S32G_SECONDARY_WAKE_SGI	15

#define S32G_XRDC_BASE		0x401A4000
#define S32G_XRDC_SIZE		0x10000

/* SRAM is actually at 0x3400_0000; we are just mirroring it in the
 * Virtual Code RAM
 */
#define S32G_SRAM_BASE		0x34000000
#define S32G_SRAM_SIZE		0x00800000
#define S32G_SRAM_END		(S32G_SRAM_BASE + S32G_SRAM_SIZE)

#define STANDBY_SRAM_BASE	(0x24000000ul)

/* Top of the 4GB of physical memory, accessible through the
 * extended memory map.
 */
#define S32G_DDR0_END		0x8ffffffff

/* Protected zone in DDR - we'll deploy BL31 there. Choose the top of the first
 * 2GB, which is reachable by the 32-bit eDMA.
 * This must also be kept in sync with U-Boot, which is expected to alter
 * the Linux device-tree.
 */
#define S32G_PMEM_END		(0xffffffff)
#define S32G_PMEM_LEN		0x00200000	/* conservatively allow 2MB */
#define S32G_PMEM_START		(S32G_PMEM_END - S32G_PMEM_LEN + 1)

/* Physical address 0x0 is actually mapped; to increase our
 * chances of detecting a 'null pointer access', use a location
 * that is currently not mapped to anything
 */
#define S32G_ERR_PTR		(0x44000000ul)

/* Note: depending on the compiler optimization level, this may or may not be
 * enough to prevent overflowing onto the adjacent SRAM image. Handle with care,
 * wear a helmet and compile with -Os.
 */
/* BL2 image in SRAM */
#define S32G_BL2_OFF_IN_SRAM	0x00300000
#define BL2_BASE		(S32G_SRAM_BASE + S32G_BL2_OFF_IN_SRAM)
/* this may be a bit too relaxed */
#define BL2_LIMIT		(S32G_SRAM_END - 1)

/* Temporary SRAM map:
 * - 0x3402_0000	U-Boot (runtime image, i.e. S32G_BL33_IMAGE_BASE)
 * - 0x3420_0000	Temporary BL31 (for development only)
 * - 0x3430_0000	BL2 (runtime image, i.e. BL2_BASE)
 */
#define TEMP_S32G_BL31_READ_ADDR_IN_SRAM	0x34200000ull

/* U-boot address in SRAM */
#define S32G_BL33_OFF_IN_SRAM	0x00020000
#define S32G_BL33_IMAGE_BASE	(S32G_SRAM_BASE + S32G_BL33_OFF_IN_SRAM)
#define S32G_BL33_LIMIT		(S32G_SRAM_END)
#define S32G_BL33_IMAGE_SIZE	(S32G_BL33_LIMIT - S32G_BL33_IMAGE_BASE)

/* BL31 location in DDR - physical addresses only, as the MMU is not
 * configured at that point yet
 */
#define BL31_BASE		(S32G_PMEM_START)
#define BL31_LIMIT		(S32G_PMEM_END)

/* FIXME value randomly chosen; should probably be revisited */
#define PLATFORM_STACK_SIZE		0x4000

#define MAX_IO_HANDLES			4
#define MAX_IO_DEVICES			3

#define PLAT_LOG_LEVEL_ASSERT		LOG_LEVEL_VERBOSE

#if defined IMAGE_BL1
/* To use in blX_platform_setup() */
#define FIRMWARE_WELCOME_STR_S32G	"This is S32G BL1\n"
#pragma warning "BL1 image is being built; you should configure it out."
#endif
#if defined IMAGE_BL31
#define FIRMWARE_WELCOME_STR_S32G_BL31	"This is S32G BL31\n"
/* To limit usage, keep these in sync with sizeof(s32g_mmap) */
#define MAX_MMAP_REGIONS		11
#define MAX_XLAT_TABLES			11
#endif
#if defined IMAGE_BL33
#pragma warning "BL33 image is being built; you should configure it out."
#endif

#endif /* PLATFORM_DEF_H */
