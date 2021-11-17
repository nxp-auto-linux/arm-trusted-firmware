/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 * This file is based on 'drivers/st/io/io_mmc.c'.
 * Copyright 2021 NXP
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

static int mmc_block_read(io_entity_t *entity, uintptr_t buffer,
			  size_t length, size_t *length_read)
{
	size_t length_bs_multiple;
	size_t length_bs_not_multiple;
	uint8_t partial_block_buffer[MMC_BLOCK_SIZE];

	*length_read = 0;

	/* Skip image loading on emulator */
	if (block_spec->length && S32G_EMU)
		return 0;

	length_bs_multiple = length & ~(MMC_BLOCK_SIZE - 1);
	if (length_bs_multiple)
		*length_read += mmc_read_blocks(block_spec->offset
							/ MMC_BLOCK_SIZE,
						buffer, length_bs_multiple);

	length_bs_not_multiple = length - length_bs_multiple;
	if (length_bs_not_multiple) {
		if (mmc_read_blocks((block_spec->offset + length_bs_multiple)
							/ MMC_BLOCK_SIZE,
				    (uintptr_t)&partial_block_buffer[0],
				    MMC_BLOCK_SIZE)
			!= MMC_BLOCK_SIZE)
		return -EIO;

		memcpy((void*)(buffer + length_bs_multiple),
		       &partial_block_buffer[0], length_bs_not_multiple);
		*length_read += length_bs_not_multiple;
	}

	if (*length_read != length)
		return -EIO;

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
