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

#include <tbbr_img_def.h>
#include <s32_platform_def.h>

#define S32G_NCORE_CAIU0_BASE_ADDR	0x50400000
#define S32G_NCORE_CAIU0_BASE_ADDR_H	(S32G_NCORE_CAIU0_BASE_ADDR >> 16)
#define NCORE_CAIUTC_OFF		0x0
#define NCORE_CAIUTC_ISOLEN_SHIFT	1
#define NCORE_CAIUTC_ISOLEN_MASK	BIT(NCORE_CAIUTC_ISOLEN_SHIFT)

/* FIXME I'm not sure this is technically correct. We do NOT have
 * cluster-level power management operations, only core and system.
 */
#define PLAT_NUM_PWR_DOMAINS		(PLATFORM_SYSTEM_COUNT + \
					 PLATFORM_CLUSTER_COUNT + \
					 PLATFORM_CORE_COUNT)

#define PLAT_PRIMARY_CPU		0x0

#define SIUL2_0_BASE_ADDR	0x4009C000UL
#define SIUL2_1_BASE_ADDR	0x44010000UL

#define GPR_BASE_ADDR		0x4007C400UL
#define GPR06_OFF		0x18U
#define GPR09_OFF		0x24U
#define GPR36_OFF		0x90U
#define CA53_RVBARADDR_MASK	(0xFFUL)
/* GPR09 */
#define CA53_0_0_RVBARADDR_39_32_OFF	(0)
#define CA53_0_1_RVBARADDR_39_32_OFF	(8)
#define CA53_1_0_RVBARADDR_39_32_OFF	(16)
#define CA53_1_1_RVBARADDR_39_32_OFF	(24)
/* GPR36 */
#define CA53_0_2_RVBARADDR_39_32_OFF	(0)
#define CA53_0_3_RVBARADDR_39_32_OFF	(8)
#define CA53_1_2_RVBARADDR_39_32_OFF	(16)
#define CA53_1_3_RVBARADDR_39_32_OFF	(24)


#define BOOT_GPR_BASE		0x4007C900UL
#define BOOT_GPR_BMR1_OFF	0
#define BOOT_RCON_MODE_MASK	0x100
#define BOOT_SOURCE_MASK	0xE0
#define BOOT_SOURCE_OFF		5
#define BOOT_SOURCE_QSPI	0
#define BOOT_SOURCE_SD		2
#define BOOT_SOURCE_MMC		3
#define INVALID_BOOT_SOURCE	1

/* GIC (re)definitions */
#define S32G274A_GIC_BASE	0x50800000
#define PLAT_GICD_BASE		S32G274A_GIC_BASE
#define S32G274A_GIC_SIZE	0x100000
/* SGI to use for kicking the secondary cores out of wfi */
#define S32G_SECONDARY_WAKE_SGI	15

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

/* Physical address 0x0 is actually mapped; to increase our
 * chances of detecting a 'null pointer access', use a location
 * that is currently not mapped to anything
 */
#define S32G_ERR_PTR		(0x44000000ul)

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

#define FIP_BASE		(S32_SRAM_END - FIP_ROFFSET)

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

#define S32G_SCMI_SHARED_MEM		0xd0000000U
#define S32G_SCMI_SHARED_MEM_SIZE	0x400000U

#define S32G_QSPI_BASE		(0x40134000ul)
#define S32G_QSPI_SIZE		(0x1000)

#define S32G_FLASH_BASE		(0x0)

#endif /* S32G_PLATFORM_DEF_H */
