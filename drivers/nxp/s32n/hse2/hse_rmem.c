/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * NXP HSE2 Driver - memory management
 *
 * Copyright 2025 NXP
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <lib/utils_def.h>

#include "hse_rmem.h"

#define HSE_NODE_SIZE    sizeof(struct node_data) /* memory block metadata */

/**
 * struct node_data - reserved memory block metadata
 * @size: buffer size, not including metadata
 * @used: node currently in use flag
 * @next_offset: offset of next block in rmem
 */
struct node_data {
	uint32_t size;
	uint8_t used;
	uint8_t reserved[3];
	uint64_t next_offset;
} __packed;

static struct node_data *mem_start;

static struct node_data *next_node(struct node_data *block)
{
	if (!block || !block->next_offset)
		return NULL;

	return (struct node_data *)((uint8_t *)mem_start + block->next_offset);
}

/**
 * hse_rmem_setup - initialize reserved memory range
 * @base addr: rmem base address
 * @mem_size: rmem size
 *
 * Return: 0 un success, -EINVAL for invalid parameter
 */
int hse_rmem_setup(uintptr_t base_addr, size_t mem_size)
{
	struct node_data **mem =  &mem_start;

	if (!base_addr || !mem_size)
		return -EINVAL;

	*mem = (struct node_data *)base_addr;
	(*mem)->size = mem_size - HSE_NODE_SIZE;
	(*mem)->next_offset = 0;
	(*mem)->used = false;

	return 0;
}

/**
 * hse_rmem_alloc - allocate buffer in reserved memory range
 * @size: buffer size
 *
 * Return: buffer virtual address on success, NULL otherwise
 */
void *hse_rmem_alloc(size_t size)
{
	struct node_data *crt_block, *best_block, *alloc_block;
	size_t best_block_size;
	uint64_t mem_start_u64;

	if (!size)
		return NULL;

	/* align size to HSE_NODE_SIZE */
	size = round_up(size, HSE_NODE_SIZE);

	crt_block = mem_start;
	best_block = NULL;
	best_block_size = mem_start->size;

	/* search for the next free node that is large enough */
	while (crt_block) {
		if (crt_block->used) {
			crt_block = next_node(crt_block);
			continue;
		}

		if (crt_block->size == (size + HSE_NODE_SIZE)) {
			best_block = crt_block;
			break;
		}

		if ((crt_block->size >= (size + HSE_NODE_SIZE)) &&
		    (crt_block->size <= best_block_size)) {
			best_block = crt_block;
			best_block_size = crt_block->size;
		}

		crt_block = next_node(crt_block);
	}

	if (!best_block)
		return NULL;

	/* found a match, split a chunk of requested size and return it */
	best_block->size = best_block->size - size - HSE_NODE_SIZE;
	alloc_block = (struct node_data *)((uint8_t *)best_block +
					   HSE_NODE_SIZE + best_block->size);
	alloc_block->size = size;
	alloc_block->used = true;

	mem_start_u64 = (uint64_t)mem_start;

	alloc_block->next_offset = best_block->next_offset;
	best_block->next_offset = (uint64_t)alloc_block - mem_start_u64;

	return (void *)((uint8_t *)alloc_block + HSE_NODE_SIZE);
}

/**
 * hse_rmem_free - free buffer from reserved memory range
 * @addr: buffer address
 */
void hse_rmem_free(void *addr)
{
	struct node_data *prev_block, *next_block, *free_block;

	if (!addr)
		return;

	/* get the node_data for the block to be freed */
	free_block = (struct node_data *)((uint8_t *)addr - HSE_NODE_SIZE);
	if (free_block == NULL)
		return;

	next_block = mem_start;

	free_block->used = false;

	prev_block = NULL;
	while ((next_block != NULL) && (next_block < free_block)) {
		prev_block = next_block;
		next_block = next_node(next_block);
	}

	if (next_node(free_block) != NULL) {
		if (!next_node(free_block)->used) {
			free_block->size += next_node(free_block)->size;
			free_block->size += HSE_NODE_SIZE;

			/* remove next_block from list */
			free_block->next_offset = next_node(free_block)->next_offset;
		}
	}

	if (prev_block != NULL) {
		if (!prev_block->used) {
			prev_block->size += free_block->size + HSE_NODE_SIZE;

			/* remove free_block from list */
			prev_block->next_offset = free_block->next_offset;
		}
	}
}

/**
 * hse_rmem_copy - memcpy routine for reserved memory
 * @dst: destination address
 * @src: source address
 * @size: size
 *
 * Return: destination buffer virtual address
 */
void *hse_rmem_copy(void *dst, const void *src, size_t size)
{
	const uint8_t *s = src;
	uint8_t *d = dst;
	const uint64_t *s64;
	uint64_t *d64;

	if (!src || !dst || src == dst || !size)
		return dst;

	/* write bytes if source OR destination are not 64bit-aligned */
	while (((uintptr_t)d & 7) || ((uintptr_t)s & 7)) {
		*d++ = *s++;
		if (!(--size))
			return dst;
	}

	/* write 64bit chunks if both source and destination are aligned */
	d64 = (uint64_t *)d;
	s64 = (uint64_t *)s;
	for (; size >= 8; size -= 8)
		*d64++ = *s64++;

	/* write bytes for the rest of the buffer */
	d = (uint8_t *)d64;
	s = (uint8_t *)s64;
	while (size-- > 0)
		*d++ = *s++;

	return dst;
}

/**
 * hse_rmem_set - memset routine for reserved memory
 * @dst: destination address
 * @fill: fill value
 * @size: size
 *
 * Return: destination buffer virtual address
 */
void *hse_rmem_set(void *dst, uint8_t fill, size_t size)
{
	uint8_t *d = dst;
	uint64_t *d64;
	uint64_t fill64 = fill;

	if (!dst || !size)
		return dst;

	/* write bytes if destination is not 64bit-aligned */
	while (((uintptr_t)d & 7)) {
		*d = fill;
		d++;
		if (!(--size))
			return dst;
	}

	/* write each byte with fill value */
	fill64 |= fill64 << 8;
	fill64 |= fill64 << 16;
	fill64 |= fill64 << 32;

	/* write 64bit chunks if destination is aligned */
	d64 = (uint64_t *)d;
	for (; size >= 8; size -= 8) {
		*d64 = fill64;
		d64++;
	}

	/* write bytes for the rest of the buffer */
	d = (uint8_t *)d64;
	while (size-- > 0)  {
		*d = fill;
		d++;
	}

	return dst;
}
