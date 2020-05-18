/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef BL31_SSRAM_H
#define BL31_SSRAM_H

#include <s32g_ivt.h>

struct ssram_ivt_sec {
	struct s32gen1_ivt ivt;
	struct s32gen1_application_boot_code app_code;
} __attribute__((packed));

_Static_assert(sizeof(struct ssram_ivt_sec) == BL31SSRAM_IVT_SIZE,
		"The value of BL31SSRAM_IVT_SIZE isn't accurate."
		"Please adjust it to reflect ssram_ivt_sec's size.");

/* The content of BL31_SSRAM stage */
extern unsigned char bl31ssram[];
extern unsigned int bl31ssram_len;

#endif
