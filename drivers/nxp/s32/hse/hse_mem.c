/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2021-2024 NXP
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <utils_def.h>

#define HSE_NODE_SIZE    sizeof(struct node_data) /* memory block metadata */

struct node_data {
	uint32_t size;
	uint8_t used;
	uint8_t reserved[3];
	uint64_t next_offset;
} __attribute__((packed));

static struct node_data *mem_start;

static struct node_data *next_node(struct node_data *block)
{
	if (!block || !block->next_offset)
		return NULL;

	return (struct node_data *)((uint8_t *)mem_start + block->next_offset);
}

int hse_mem_setup(uintptr_t base_addr, size_t mem_size)
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

void *hse_mem_alloc(size_t size)
{
	struct node_data *crt_block;
	struct node_data *best_block;
	struct node_data *alloc_block;
	size_t best_block_size;
	uint64_t mem_start_u64;

	/* align size to HSE_NODE_SIZE */
	size = round_up(size, HSE_NODE_SIZE);
	if (!size)
		return NULL;

	crt_block = mem_start;
	best_block = NULL;
	best_block_size = mem_start->size;

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

void hse_mem_free(void *addr)
{
	struct node_data *prev_block;
	struct node_data *next_block;
	struct node_data *free_block;

	if (addr == NULL)
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

void *hse_memcpy(void *dest, const void *src, size_t size)
{
	const uint8_t *s = src;
	uint8_t *d = dest;
	const uint64_t *s64;
	uint64_t *d64;

	if (!size)
		return dest;

	/* write bytes if source OR destination are not 64bit-aligned */
	while (((uintptr_t)d & 7) || ((uintptr_t)s & 7)) {
		*d++ = *s++;
		if (!(--size))
			return dest;
	}

	/* write 64bit if both source and destination are aligned */
	d64 = (uint64_t *)d;
	s64 = (uint64_t *)s;
	for (; size >= 8; size -= 8)
		*d64++ = *s64++;

	/* write bytes for the rest of the buffer */
	d = (uint8_t *)d64;
	s = (uint8_t *)s64;
	while (size-- > 0)
		*d++ = *s++;

	return dest;
}

/**
 * hse_virt_to_phys - get physical address from virtual address
 * @addr: virtual address in the reserved memory range
 */
uintptr_t hse_virt_to_phys(const void *addr)
{
	return (uintptr_t)addr;
}
