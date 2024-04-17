// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/*
 * Copyright 2021-2022, 2024 NXP
 */
#include "s32cc_dt.h"
#include <common/debug.h>
#include <drivers/nxp/s32/ocotp.h>
#include <errno.h>
#include <inttypes.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <stdbool.h>
#include <stddef.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

#define OCOTP_WORD(X)		BIT(X)
#define OCOTP_WORD_RANGE(L, H)	GENMASK(H, L)

#define CTRL_SYS		0x0
#define   CTRL_AUTH_KEY		(0x12 << 16)
#define   CTRL_RD_WR(X)		((X) << 2)
#define   CTRL_READ_FUSE	1
#define ADDR_SYS		0x4
#define RDATA_SYS		0xC
#define STATUS_SYS		0x50
#define   STATUS_BUSY		BIT(0)
#define   STATUS_CRC_FAIL	BIT(1)
#define   STATUS_ERROR		BIT(2)

struct s32gen1_ocotp {
	struct dt_node_info dt_info;
};

struct s32gen1_fuse_map {
	const uint32_t *map;
	size_t n_banks;
};

static const uint32_t s32g_fuse_map[] = {
	[0] = OCOTP_WORD_RANGE(2, 6),
	[1] = OCOTP_WORD_RANGE(5, 7),
	[2] = OCOTP_WORD_RANGE(0, 4),
	[4] = OCOTP_WORD(6),
	[5] = OCOTP_WORD_RANGE(1, 2),
	[6] = OCOTP_WORD(7),
	[7] = OCOTP_WORD_RANGE(0, 1),
	[11] = OCOTP_WORD_RANGE(0, 7),
	[12] = OCOTP_WORD_RANGE(0, 2) | OCOTP_WORD(7),
	[13] = OCOTP_WORD_RANGE(2, 4),
	[14] = OCOTP_WORD(1) | OCOTP_WORD(4) | OCOTP_WORD(5),
	[15] = OCOTP_WORD_RANGE(5, 7),
};

static const struct s32gen1_fuse_map s32g_map = {
	.map = s32g_fuse_map,
	.n_banks = ARRAY_SIZE(s32g_fuse_map),
};

static struct s32gen1_ocotp gocotp = {
	. dt_info = {
		.status = DT_DISABLED,
	},
};

static uint32_t get_bank_index(int offset)
{
	return (offset - S32GEN1_OCOTP_BANK_OFFSET) / S32GEN1_OCOTP_BANK_SIZE;
}

static uint32_t get_word_index(int offset)
{
	return offset % S32GEN1_OCOTP_BANK_SIZE / S32GEN1_OCOTP_WORD_SIZE;
}

static bool is_valid_word(const struct s32gen1_fuse_map *map,
			  uint32_t bank, uint32_t word)
{
	if (bank >= map->n_banks)
		return false;

	return !!(map->map[bank] & OCOTP_WORD(word));
}

static uint32_t wait_if_busy(uintptr_t base)
{
	uint32_t status;

	do {
		status = mmio_read_32(base + STATUS_SYS);
	} while (status & STATUS_BUSY);

	return status;
}

static int read_ocotp(uintptr_t base, uint32_t reg, uint32_t *val)
{
	uint32_t status;

	status = wait_if_busy(base);

	if (status & STATUS_ERROR) {
		mmio_write_32(base + STATUS_SYS, status);

		status = wait_if_busy(base);
	}

	status = mmio_read_32(base + STATUS_SYS);
	if (status & STATUS_ERROR) {
		ERROR("Failed to clear OCOTP\n");
		return -EIO;
	}

	mmio_write_32(base + ADDR_SYS, reg);
	mmio_write_32(base + CTRL_SYS,
		      CTRL_AUTH_KEY | CTRL_RD_WR(CTRL_READ_FUSE));

	status = wait_if_busy(base);
	if (status & STATUS_ERROR)
		return -EIO;

	*val = mmio_read_32(base + RDATA_SYS);

	return 0;
}

int s32gen1_ocotp_read(int offset, uint32_t *val)
{
	uint32_t bank, word;

	if (gocotp.dt_info.status != DT_ENABLED)
		return -ENXIO;

	if (offset < 0)
		return -EINVAL;

	bank = get_bank_index(offset);
	word = get_word_index(offset);

	if (!is_valid_word(&s32g_map, bank, word)) {
		ERROR("OCOTP: [bank %" PRIu32 " word %" PRIu32 "] is not a valid fuse\n",
			bank, word);
		return -EINVAL;
	}

	return read_ocotp(gocotp.dt_info.base, offset, val);
}

int s32gen1_ocotp_init(void *fdt, int fdt_offset)
{
	uint64_t reg_base;
	size_t reg_size;
	int ret;

	/* Register active nodes only */
	dt_fill_device_info(&gocotp.dt_info, fdt_offset);
	if (gocotp.dt_info.status != DT_ENABLED)
		return -ENXIO;

	reg_base = gocotp.dt_info.base;
	reg_size = round_up(gocotp.dt_info.reg_size, PAGE_SIZE);

	ret = mmap_add_dynamic_region(reg_base, reg_base,
				      reg_size, MT_DEVICE | MT_RW);

	if (ret) {
		ERROR("Failed to map ocotp region base_pa: 0x%" PRIx64 ", error: %d\n",
		      reg_base, ret);
		return ret;
	}

	return 0;
}
