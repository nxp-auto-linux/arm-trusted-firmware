/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 * This file is based on 'drivers/st/io/io_mmc.c'.
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/nxp/s32/io/io_mmc.h>
#include <drivers/mmc.h>

static io_block_spec_t *block_spec;
static const io_dev_info_t mmc_dev_info;

static int mmc_dev_len(io_entity_t *entity, size_t *length)
{
	*length = (block_spec->length) & ~(MMC_BLOCK_MASK);
	return 0;
}

static io_type_t device_type_mmc(void)
{
	return IO_TYPE_MMC;
}

static int mmc_dev_open(const uintptr_t init_params, io_dev_info_t **dev_info)
{
	assert(dev_info);
	*dev_info = (io_dev_info_t *)&mmc_dev_info;

	return 0;
}

static const io_dev_connector_t mmc_dev_connector = {
	.dev_open = mmc_dev_open
};

static int mmc_dev_init(io_dev_info_t *dev_info, const uintptr_t init_params)
{
	return 0;
}

static int mmc_dev_close(io_dev_info_t *dev_info)
{
	return 0;
}

static int mmc_block_open(io_dev_info_t *dev_info, const  uintptr_t spec,
			  io_entity_t *entity)
{
	block_spec = (io_block_spec_t *)spec;
	return 0;
}

static int mmc_block_seek(io_entity_t *entity, int mode,
			  signed long long offset)
{
	return -ENOTSUP;
}

static int read_mmc_range(size_t blocks_off, size_t len, uintptr_t dest)
{
	size_t bytes;

	len = ROUND_TO_MMC_BLOCK_SIZE(len);

	/**
	 * Flush previously loaded content in case 'dest'
	 * isn't cache line aligned
	 */
	flush_dcache_range(dest, len);
	inv_dcache_range(dest, len);
	bytes = mmc_read_blocks(blocks_off / MMC_BLOCK_SIZE, dest, len);
	if (bytes != len)
		return -EIO;

	return 0;

}
static int mmc_block_read(io_entity_t *entity, uintptr_t buffer,
			  size_t length, size_t *length_read)
{
	size_t offset, part_offset;
	size_t copy_off, copy_len, copy_block;
	uintptr_t dest_addr;
	bool partial;
	int ret;
	uint8_t partial_block_buffer[MMC_BLOCK_SIZE];

	*length_read = length;

	offset = block_spec->offset;
	while (length > 0) {
		partial = false;
		copy_len = MMC_BLOCK_SIZE;

		/* Check if offset is block aligned */
		part_offset = offset & ~(MMC_BLOCK_SIZE - 1);
		if (part_offset != offset)
			copy_len = MMC_BLOCK_SIZE - offset % MMC_BLOCK_SIZE;

		/* Partial copy of the block */
		if (length < MMC_BLOCK_SIZE)
			copy_len = length;

		if (part_offset != offset || copy_len < MMC_BLOCK_SIZE) {
			partial = true;
			copy_off = offset - part_offset;
			copy_block = part_offset;
			dest_addr = (uintptr_t)&partial_block_buffer[0];
		} else {
			copy_off = 0x0;
			copy_block = offset;
			copy_len = length & ~(MMC_BLOCK_SIZE - 1);
			dest_addr = buffer;
		}

		ret = read_mmc_range(copy_block, copy_len, dest_addr);
		if (ret) {
			*length_read = 0;
			return ret;
		}

		if (partial)
			memcpy((void *)buffer, (void *)(dest_addr + copy_off),
			       copy_len);

		length -= copy_len;
		offset += copy_len;
		buffer += copy_len;
	}

	return 0;
}

static int mmc_block_close(io_entity_t *entity)
{
	return 0;
}

int register_io_dev_mmc(const io_dev_connector_t **dev_con)
{
	int rc;

	assert(dev_con);

	rc = io_register_device(&mmc_dev_info);
	if (!rc)
		*dev_con = &mmc_dev_connector;

	return rc;
}

static const io_dev_funcs_t mmc_dev_funcs = {
	.type = device_type_mmc,
	.open = mmc_block_open,
	.seek = mmc_block_seek,
	.size = mmc_dev_len,
	.read = mmc_block_read,
	.write = NULL,
	.close = mmc_block_close,
	.dev_init = mmc_dev_init,
	.dev_close = mmc_dev_close,
};

static const io_dev_info_t mmc_dev_info = {
	.funcs = &mmc_dev_funcs,
	.info = 0,
};
