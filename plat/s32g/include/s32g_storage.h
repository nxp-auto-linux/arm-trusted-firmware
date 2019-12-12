/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G_STORAGE_H
#define S32G_STORAGE_H

/* Temporary SRAM map:
 * - 0x3402_0000	U-Boot (runtime image, i.e. S32G_BL33_IMAGE_BASE)
 * - 0x3420_0000	Temporary BL31 (for development only)
 * - 0x3430_0000	BL2 (runtime image, i.e. BL2_BASE)
 * - 0x3440_0000	BL31 (runtime image, i.e. BL31_BASE)
 */
#define TEMP_S32G_BL31_READ_ADDR_IN_SRAM	0x34200000ull

enum s32g_boot_source {
	S32G_SRAM_BOOT,
	/* TODO add FIP, QSPI, SD/MMC */
};

struct plat_io_policy {
	uintptr_t *dev_handle;
	uintptr_t image_spec;
	int (*check)(const uintptr_t spec);
};

void s32g_io_setup(void);

#endif /* S32G_STORAGE_H */
