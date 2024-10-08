/*
 * Copyright 2019-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32CC_STORAGE_H
#define S32CC_STORAGE_H

#include <tools_share/uuid.h>
#include <string.h>

struct plat_io_policy {
	uintptr_t *dev_handle;
	uintptr_t image_spec;
	int (*check)(const uintptr_t spec);
};

void s32_io_setup(void);

#endif /* S32CC_STORAGE_H */
