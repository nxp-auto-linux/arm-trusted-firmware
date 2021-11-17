/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <lib/utils_def.h>
#include <stddef.h>

#define INIT_MEM_POOL(ARRAY)			\
	{					\
		.data = &(ARRAY),		\
		.n_elem = ARRAY_SIZE(ARRAY),	\
		.el_size = sizeof((ARRAY)[0]),	\
		.fill_level = 0x0,		\
	}

struct memory_pool {
	void *data;
	size_t n_elem;
	size_t el_size;
	size_t fill_level;
};

void *alloc_mem_pool_elem(struct memory_pool *pool);

#endif
