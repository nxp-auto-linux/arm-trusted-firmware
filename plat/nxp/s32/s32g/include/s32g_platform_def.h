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

#define SIUL2_1_BASE_ADDR	0x44010000UL
#define S32_MAX_I2C_MODULES 5

/* GPR36 */
#define CA53_0_2_RVBARADDR_39_32_OFF	(0)
#define CA53_0_3_RVBARADDR_39_32_OFF	(8)
#define CA53_1_2_RVBARADDR_39_32_OFF	(16)
#define CA53_1_3_RVBARADDR_39_32_OFF	(24)

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

#if defined IMAGE_BL31
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
