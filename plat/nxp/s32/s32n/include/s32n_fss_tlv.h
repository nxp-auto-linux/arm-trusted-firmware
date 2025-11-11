/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_FSS_TLV_H
#define S32N_FSS_TLV_H

/* 0xFF_F001 SoC info. */
struct fss_soc_product {
	uint8_t name_len;
	const char *name;
} __packed;

struct fss_soc_family {
	uint8_t name_len;
	const char *name;
} __packed;

/* 0xFF_F003 Boot info. */
struct fss_boot_info {
	uint32_t bsr;
	uint32_t besr;
	uint8_t version;
	uint8_t reset_reason[3];
	uint8_t primary_or_backup;
	uint8_t boot_desc_error[3];
} __packed;

struct fss_aps_range {
	uint32_t base;
	uint32_t end;
} __packed;

/* 0xFF_F00E APS peripheral base address. */
struct fss_aps_local_addr {
	uint32_t aps_count;
	struct fss_aps_range ranges[];
} __packed;

int fss_tlv_get_arm_timer_freq(uint32_t *out_hz);

/* The following functions return read-only pointers into TLV. */
int fss_tlv_get_soc_info(struct fss_soc_product *prod,
		struct fss_soc_family *family);
int fss_tlv_get_boot_info(const struct fss_boot_info **boot_info);
int fss_tlv_get_xspi_dump(const uint8_t **out_ptr, size_t *out_len);
int fss_tlv_get_aps_local_addr(const struct fss_aps_local_addr **aps_local_addr);

#endif /* S32N_FSS_TLV_H */
