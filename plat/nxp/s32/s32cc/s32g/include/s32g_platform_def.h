/*
 * Copyright 2019-2024 NXP
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
#include <s32cc_platform_def.h>

#define SIUL2_1_BASE_ADDR	(0x44010000UL)
#define SIUL2_1_SIZE		(0x17B0)
#define S32_MAX_I2C_MODULES 5

/* GPR36 */
#define CA53_0_2_RVBARADDR_39_32_OFF	(0)
#define CA53_0_3_RVBARADDR_39_32_OFF	(8)
#define CA53_1_2_RVBARADDR_39_32_OFF	(16)
#define CA53_1_3_RVBARADDR_39_32_OFF	(24)

#define SSRAMC_BASE_ADDR        (0x44028000UL)

#define S32G_SSRAM_BASE		(0x24000000)
#define S32G_SSRAM_SIZE		(0x8000)
#define S32G_SSRAM_LIMIT	(S32G_SSRAM_BASE + S32G_SSRAM_SIZE)

#define WKPU_BASE_ADDR		(0x40090000ul)
#define WKPU_SIZE		(0x38)

/* Physical address 0x0 is actually mapped; to increase our
 * chances of detecting a 'null pointer access', use a location
 * that is currently not mapped to anything
 */
#define S32G_ERR_PTR		(0x44000000ul)

#define BL31SRAM_BASE		BL2_BASE
#define BL31SRAM_MAX_PAGES	50
#define BL31SRAM_SIZE		(BL31SRAM_MAX_PAGES * PAGE_SIZE)
#define BL31SRAM_LIMIT		(BL31SRAM_BASE + BL31SRAM_SIZE)

#define BL31SSRAM_MAILBOX	(S32G_SSRAM_BASE)

#define MAX_MMAP_REGIONS		52
#ifndef MAX_XLAT_TABLES
#define MAX_XLAT_TABLES			28
#endif

#if defined IMAGE_BL33
#pragma warning "BL33 image is being built; you should configure it out."
#endif

#define SIUL2_1_MSCR_START	112
#define SIUL2_1_MSCR_END	190
#define SIUL2_1_IMCR_START	631
#define SIUL2_1_IMCR_END	1007

#endif /* S32G_PLATFORM_DEF_H */
