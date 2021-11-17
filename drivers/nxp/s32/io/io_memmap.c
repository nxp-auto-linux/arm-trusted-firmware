/*
 * Copyright (c) 2014-2018, ARM Limited and Contributors. All rights reserved.
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <string.h>
#include <platform_def.h>
#include <common/debug.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_memmap.h>
#include <drivers/io/io_storage.h>
#include <lib/utils.h>

/* As we need to be able to keep state for seek, only one file can be open
 * at a time. Make this a structure and point to the entity->info. When we
 * can malloc memory we can change this to support more open files.
 */
struct file_state {
	/* Use the 'in_use' flag as any value for base and file_pos could be
	 * valid.
	 */
	uint8_t		in_use;
	uintptr_t	base;
	size_t		file_pos;
	size_t		size;
};

static struct file_state current_file = {0};

static io_type_t device_type_memmap(void)
{
	return IO_TYPE_MEMMAP;
}

static const io_dev_info_t memmap_dev_info;

static int memmap_dev_open(const uintptr_t dev_spec __unused,
			   io_dev_info_t **dev_info)
{
	assert(dev_info);
	*dev_info = (io_dev_info_t *)&memmap_dev_info;

	return 0;
}

static const io_dev_connector_t memmap_dev_connector = {
	.dev_open = memmap_dev_open
};

static int memmap_dev_close(io_dev_info_t *dev_info)
{
	return 0;
}

static int memmap_block_open(io_dev_info_t *dev_info, const uintptr_t spec,
			     io_entity_t *entity)
{
	int result = -ENOMEM;
	const io_block_spec_t *block_spec = (io_block_spec_t *)spec;

	/* Since we need to track open state for seek() we only allow one open
	 * spec at a time. When we have dynamic memory we can malloc and set
	 * entity->info.
	 */
	if (current_file.in_use == 0) {
		assert(block_spec);
		assert(entity);

		current_file.in_use = 1;
		current_file.base = block_spec->offset;
		current_file.file_pos = 0;
		current_file.size = block_spec->length;
		entity->info = (uintptr_t)&current_file;
		result = 0;
	} else {
		WARN("A Memmap device is already active. Close first.\n");
	}

	return result;
}

static int memmap_block_seek(io_entity_t *entity, int mode,
			     signed long long offset)
{
	int result = -ENOENT;
	struct file_state *fp;

	/* We only support IO_SEEK_SET for the moment. */
	if (mode == IO_SEEK_SET) {
		assert(entity);
		fp = (struct file_state *)entity->info;
		assert((offset >= 0) && (offset < fp->size));
		fp->file_pos = offset;
		result = 0;
	}

	return result;
}

static int memmap_block_len(io_entity_t *entity, size_t *length)
{
	assert(entity);
	assert(length);

	*length = ((struct file_state *)entity->info)->size;

	return 0;
}

static void s32g_memcpy(uint8_t *dest, const uint8_t *src, size_t count)
{
	uint64_t *dest64, *src64;
	uint32_t *dest32, *src32;

	if (src == dest)
		return;

	dest64 = (uint64_t *)dest;
	src64 = (uint64_t *)src;

	/**
	 * while all data is aligned (common case), copy an uint64_t at a time
	 */
	if (!(((uintptr_t)dest64 | (uintptr_t)src64) & (sizeof(*dest64) - 1))) {
		while (count >= sizeof(*dest64)) {
			*dest64++ = *src64++;
			count -= sizeof(*dest64);
		}
	}

	dest32 = (uint32_t *)dest64;
	src32 = (uint32_t *)src64;
	if (!(((uintptr_t)dest32 | (uintptr_t)src32) & (sizeof(*dest32) - 1))) {
		while (count >= sizeof(*dest32)) {
			*dest32++ = *src32++;
			count -= sizeof(*dest32);
		}
	}

	dest = (uint8_t *)dest32;
	src = (uint8_t *)src32;

	while (count--)
		*dest++ = *src++;
}

static int memmap_block_read(io_entity_t *entity, uintptr_t buffer,
			     size_t length, size_t *length_read)
{
	struct file_state *fp;
	size_t pos_after;

	assert(entity);
	assert(length_read);

	fp = (struct file_state *)entity->info;
	pos_after = fp->file_pos + length;
	assert((pos_after >= fp->file_pos) && (pos_after <= fp->size));

	s32g_memcpy((uint8_t *)buffer, (uint8_t *)(fp->base + fp->file_pos),
		    length);

	*length_read = length;
	fp->file_pos = pos_after;

	return 0;
}

static int memmap_block_write(io_entity_t *entity, const uintptr_t buffer,
			      size_t length, size_t *length_written)
{
	struct file_state *fp;
	size_t pos_after;

	assert(entity);
	assert(length_written);

	fp = (struct file_state *)entity->info;
	pos_after = fp->file_pos + length;
	assert((pos_after >= fp->file_pos) && (pos_after <= fp->size));

	memcpy((void *)(fp->base + fp->file_pos), (void *)buffer, length);

	*length_written = length;
	fp->file_pos = pos_after;

	return 0;
}

static int memmap_block_close(io_entity_t *entity)
{
	assert(entity);

	entity->info = 0;

	/* This would be a mem free() if we had malloc.*/
	zeromem((void *)&current_file, sizeof(current_file));

	return 0;
}

int register_io_dev_memmap(const io_dev_connector_t **dev_con)
{
	int result;

	assert(dev_con);

	result = io_register_device(&memmap_dev_info);
	if (result == 0)
		*dev_con = &memmap_dev_connector;

	return result;
}

static const io_dev_funcs_t memmap_dev_funcs = {
	.type = device_type_memmap,
	.open = memmap_block_open,
	.seek = memmap_block_seek,
	.size = memmap_block_len,
	.read = memmap_block_read,
	.write = memmap_block_write,
	.close = memmap_block_close,
	.dev_init = NULL,
	.dev_close = memmap_dev_close,
};

static const io_dev_info_t memmap_dev_info = {
	.funcs = &memmap_dev_funcs,
	.info = (uintptr_t)NULL
};
