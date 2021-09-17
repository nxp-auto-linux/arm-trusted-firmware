/*
 * Copyright 2019-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * TODO: review these definitions. Some of them might not have been
 * correctly and thorougly understood at the time of this writing.
 */

#ifndef S32G_PLATFORM_DEF_H
#define S32G_PLATFORM_DEF_H

#include <common_def.h>
#include <tbbr_img_def.h>

#define SIZE_1M		(0x100000)

/* MPIDR_EL1 for the four A53 cores is as follows:
 *	A53_0_cpu0:	0x8000_0000
 *	A53_0_cpu1:	0x8000_0001
 *	A53_0_cpu2:	0x8000_0002
 *	A53_0_cpu3:	0x8000_0003
 *	A53_1_cpu0:	0x8000_0100
 *	A53_1_cpu1:	0x8000_0101
 *	A53_1_cpu2:	0x8000_0102
 *	A53_1_cpu3:	0x8000_0103
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

#define BOOT_GPR_BASE		0x4007C900UL
#define BOOT_GPR_BMR1_OFF	0
#define BOOT_RCON_MODE_MASK	0x100
#define BOOT_SOURCE_MASK	0xE0
#define BOOT_SOURCE_OFF		5
#define BOOT_SOURCE_QSPI	0
#define BOOT_SOURCE_SD		2
#define BOOT_SOURCE_MMC		3
#define INVALID_BOOT_SOURCE	1

#define S32G_XRDC_0_PAC_0_BASE_ADDR	0x40000000ULL
#define S32G_XRDC_0_PAC_0_SIZE		SIZE_1M

/* GIC (re)definitions */
#define S32G274A_GIC_BASE	0x50800000
#define PLAT_GICD_BASE		S32G274A_GIC_BASE
#define PLAT_GICR_BASE		(S32G274A_GIC_BASE + 0x80000)
#define S32G274A_GIC_SIZE	0x100000
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

#define S32G_SSRAM_BASE		(0x24000000)
#define S32G_SSRAM_SIZE		(0x8000)
#define S32G_SSRAM_LIMIT	(S32G_SSRAM_BASE + S32G_SSRAM_SIZE)

/* RTC definitions space */
#define S32G_RTC_BASE		0x40060000
#define RTC_RTCC_OFFSET		0x4
#define RTC_RTCS_OFFSET		0x8
#define RTC_RTCS_RTCF		BIT(29)
#define RTC_APIVAL_OFFSET	0x10
#define RTC_RTCVAL_OFFSET	0x14

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
#define S32G_PMEM_LEN		(2 * SIZE_1M)	/* conservatively allow 2MB */
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
#define BOOTROM_ADMA_RSRVD_BASE	(0x343ff000)
#define BL2_LIMIT		(BOOTROM_ADMA_RSRVD_BASE - 1)

#define BL31SRAM_BASE		BL2_BASE
#define BL31SRAM_MAX_PAGES	50
#define BL31SRAM_SIZE		(BL31SRAM_MAX_PAGES * PAGE_SIZE)
#define BL31SRAM_LIMIT		(BL31SRAM_BASE + BL31SRAM_SIZE)

#define BL31SSRAM_IVT		(S32G_SSRAM_BASE)
#define BL31SSRAM_IVT_SIZE	(0x140)
#define BL31SSRAM_MAILBOX	(BL31SSRAM_IVT + BL31SSRAM_IVT_SIZE)
#define BL31SSRAM_OFFSET	(0x600)
#define BL31SSRAM_BASE		(S32G_SSRAM_BASE + BL31SSRAM_OFFSET)
#define BL31SSRAM_LIMIT		S32G_SSRAM_LIMIT
#define BL31SSRAM_MAX_CODE_SIZE	(S32G_SSRAM_LIMIT - BL31SSRAM_BASE)
#define BL31SSRAM_STACK_SIZE	0x1000

#define DTB_SIZE		(BL2_BASE - DTB_BASE)

/* U-boot addresses in SRAM. BL33_DTB and BL33_ENTRYPOINT must be kept in
 * sync with u-boot's CONFIG_DTB_SRAM_ADDR and CONFIG_SYS_TEXT_BASE.
 */
#define S32G_BL33_IMAGE_SIZE	(5 * SIZE_1M)
/* Leave a gap between BL33 and BL31 to avoid MMU entries merge */
#define BL33_BASE		(S32G_PMEM_START - S32G_BL33_IMAGE_SIZE - \
				 SIZE_1M)
#define BL33_DTB		(BL33_BASE + 0x90000)
#define BL33_ENTRYPOINT		(BL33_BASE + 0xa0000)
#define S32G_BL33_IMAGE_BASE	(BL33_DTB)
#define S32G_BL33_LIMIT		(BL33_BASE + S32G_BL33_IMAGE_SIZE)

/* BL31 location in DDR - physical addresses only, as the MMU is not
 * configured at that point yet
 */
#define BL31_BASE		(S32G_PMEM_START)
#define BL31_LIMIT		(S32G_PMEM_END)

/* BL32 location in DDR - 22MB
 * 20 MB for optee_os (optee_os itself + TA mappings during their execution)
 * 2 MB for shared memory between optee and linux kernel
 *
 * Depending on the intensity of usage of TAs and their sizes,
 * these values can be further shrunk. The current values are preliminary.
 */
#define S32G_BL32_SIZE		0x01600000
#define S32G_BL32_BASE		(BL31_BASE - S32G_BL32_SIZE)
#define S32G_BL32_LIMIT		(BL31_BASE)

#define FIP_BASE		(S32G_SRAM_END - FIP_MAXIMUM_SIZE)

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
#define MAX_MMAP_REGIONS		17
#define MAX_XLAT_TABLES			(MAX_MMAP_REGIONS + BL31SRAM_MAX_PAGES)
#endif

#if defined IMAGE_BL2
#define MAX_MMAP_REGIONS		17
#define MAX_XLAT_TABLES			(MAX_MMAP_REGIONS + BL31SRAM_MAX_PAGES)
#endif
#if defined IMAGE_BL33
#pragma warning "BL33 image is being built; you should configure it out."
#endif

#define S32G_LINFLEX0_BASE	(0x401C8000ul)
#define S32G_LINFLEX0_SIZE	(0x4000)
#define S32G_UART_BASE		S32G_LINFLEX0_BASE
#define S32G_UART_SIZE		S32G_LINFLEX0_SIZE

#define S32G_SCMI_SHARED_MEM		0xd0000000U
#define S32G_SCMI_SHARED_MEM_SIZE	0x400000U

#define S32G_QSPI_BASE		(0x40134000ul)
#define S32G_QSPI_SIZE		(0x1000)

#define S32G_FLASH_BASE		(0x0)

#endif /* S32G_PLATFORM_DEF_H */
