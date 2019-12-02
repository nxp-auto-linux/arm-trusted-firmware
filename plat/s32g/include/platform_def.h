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

#define S32G_CACHE_WRITEBACK_SHIFT	6
#define CACHE_WRITEBACK_GRANULE		(1 << S32G_CACHE_WRITEBACK_SHIFT)
#define PLAT_PHY_ADDR_SPACE_SIZE        (1ull << 36)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(1ull << 32)

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
/* Generic timer frequency; this goes directly into CNTFRQ_EL0 */
#if defined S32G_VIRTUAL_PLATFORM
/* Dummy value, to make time passing more palatable on the functional sim.
 * We suspect the sim incorrectly further divides the clock signal, so we
 * manually (and approximately) adjust for that.
 */
#define COUNTER_FREQUENCY	0xC0000
#else
/* 5MHz; this is based on the assumption that GPR00[CA53_COUNTER_CLK_DIV_VAL]
 * contains the reset value of 0x7, hence producing a divider value of 8,
 * applied to the FXOSC frequency of 40MHz
 */
#define COUNTER_FREQUENCY	0x004C4B40
#endif

#define SIUL2_0_BASE_ADDR	0x4009C000UL
#define SIUL2_1_BASE_ADDR	0x44010000UL

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
#define S32G_SRAM_SIZE		0x00A00000
#define S32G_DDR0_BASE		0x80000000
/* FIXME this should be a compile-time option; in addition, on S32G we actually
 * have 2 DDR controllers
 */
#define S32G_DDR0_END		0x90000000	/* Keep in sync with u-boot! */
#define S32G_DDR_SIZE		(S32G_DDR0_END - S32G_DDR0_BASE)

/* Protected zone at the very top of DDR for our future use */
#define S32G_PMEM_END		(S32G_DDR0_BASE + S32G_DDR_SIZE)
#define S32G_PMEM_LEN		0x00200000	/* 2MB */
#define S32G_PMEM_START		(S32G_PMEM_END - S32G_PMEM_LEN)

/*
 * Memory layout macros
 */

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
#define S32G_BL2_OFF_IN_SRAM	0x00700000
#define BL2_BASE		(S32G_SRAM_BASE + S32G_BL2_OFF_IN_SRAM)
#define BL2_LIMIT		(S32G_SRAM_BASE + S32G_BL31_OFF_IN_SRAM - 1)

/* BL31 and BL33 location in SRAM
 */

/* BL31 is located *after* BL32 in SRAM, where there is more space potentially
 * allowing us to compile the TF-A with -O0 without overlapping with U-Boot;
 * also, U-Boot will be able to reclaim the beginning of SRAM for its MMU
 * tables without overwriting our exception vectors
 */
#define S32G_BL31_OFF_IN_SRAM		0x00800000
#define BL31_LIMIT			(S32G_SRAM_BASE + S32G_SRAM_SIZE - 1)
/* U-boot address in SRAM */
#define S32G_BL33_OFF_IN_SRAM		0x20000
#define S32G_BL33_IMAGE_BASE		(S32G_SRAM_BASE + S32G_BL33_OFF_IN_SRAM)

#define BL31_BASE			(S32G_SRAM_BASE + S32G_BL31_OFF_IN_SRAM)

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
#define MAX_MMAP_REGIONS		8
#define MAX_XLAT_TABLES			8
#endif
#if defined IMAGE_BL33
#pragma warning "BL33 image is being built; you should configure it out."
#endif

#endif /* PLATFORM_DEF_H */
