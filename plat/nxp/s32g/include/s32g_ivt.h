/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G_IVT_H
#define S32G_IVT_H

#include <stdint.h>

#define BCW_BOOT_TARGET_A53_0           BIT(0)
#define IVT_TAG				0xd1
#define IVT_LEN				0x01
#define APPLICATION_BOOT_CODE_TAG	0xd5
#define APPLICATION_BOOT_CODE_VERSION	0x60

struct s32gen1_ivt {
	uint8_t  tag;
	uint16_t length;
	uint8_t  version;
	uint8_t  reserved1[4];
	uint32_t self_test_dcd_pointer;
	uint32_t self_test_dcd_pointer_backup;
	uint32_t dcd_pointer;
	uint32_t dcd_pointer_backup;
	uint32_t hse_h_firmware_pointer;
	uint32_t hse_h_firmware_pointer_backup;
	uint32_t application_boot_code_pointer;
	uint32_t application_boot_code_pointer_backup;
	uint32_t boot_configuration_word;
	uint32_t lifecycle_configuration_word;
	uint8_t  reserved2[4];
	uint8_t  reserved_for_hse_h_fw[32];
	uint8_t  reserved3[156];
	uint32_t gmac[4];
} __attribute__((packed));

_Static_assert(sizeof(struct s32gen1_ivt) == 0x100,
		"According to S32G274A RM the IVT structure"
		"must have 0x100 bytes");

struct s32gen1_application_boot_code {
	uint8_t  tag;
	uint8_t  reserved1[2];
	uint8_t  version;
	uint32_t ram_start_pointer;
	uint32_t ram_entry_pointer;
	uint32_t code_length;
	uint32_t auth_mode;
	uint8_t  reserved2[44];
	uint8_t  code[0];
} __attribute__((packed));

#endif

