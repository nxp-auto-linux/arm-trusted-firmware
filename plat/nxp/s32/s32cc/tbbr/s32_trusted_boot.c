/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat/common/platform.h>

/* Only stub functions as we do not use an ROTPK or NV Counters */

int plat_get_rotpk_info(void *cookie, void **key_ptr, unsigned int *key_len,
			unsigned int *flags)
{
	*flags = ROTPK_NOT_DEPLOYED;
	return 0;
}

int plat_get_nv_ctr(void *cookie, unsigned int *nv_ctr)
{
	*nv_ctr = 0;
	return 0;
}

int plat_set_nv_ctr(void *cookie, unsigned int nv_ctr)
{
	return 1;
}

int plat_get_mbedtls_heap(void **heap_addr, size_t *heap_size)
{
	return get_mbedtls_heap_helper(heap_addr, heap_size);
}
