/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <memory_pool.h>

void *alloc_mem_pool_elem(struct memory_pool *pool)
{
	void *ptr;

	if (!pool)
		return NULL;

	if (pool->fill_level >= pool->n_elem)
		return NULL;

	ptr = pool->fill_level * pool->el_size + pool->data;
	pool->fill_level++;

	return ptr;
}
