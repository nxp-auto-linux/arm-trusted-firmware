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

/* Return 0 for equal uuids. */
static inline int compare_uuids(const uuid_t *uuid1, const uuid_t *uuid2)
{
	return memcmp(uuid1, uuid2, sizeof(uuid_t));
}

void set_image_spec(const uuid_t *uuid, uint64_t size, uint64_t offset);
void set_fip_hdr_spec(void);
void dump_images_spec(void);
size_t get_image_max_offset(void);
void invalidate_qspi_ahb(void);

#endif /* S32CC_STORAGE_H */
