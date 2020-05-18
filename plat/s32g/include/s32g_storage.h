/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G_STORAGE_H
#define S32G_STORAGE_H

enum s32g_boot_source {
	S32G_MMC_BOOT,
	/* TODO add FIP, QSPI, SD/MMC */
};

struct plat_io_policy {
	uintptr_t *dev_handle;
	uintptr_t image_spec;
	int (*check)(const uintptr_t spec);
};

void s32g_io_setup(void);

#endif /* S32G_STORAGE_H */
