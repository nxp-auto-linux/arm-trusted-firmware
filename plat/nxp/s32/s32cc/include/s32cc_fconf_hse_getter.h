/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FCONF_HSE_GETTER_H
#define FCONF_HSE_GETTER_H

#include <assert.h>
#include <lib/fconf/fconf.h>

#define s32cc__hse_rmem_getter(slot) __extension__ ({	\
	assert((slot) < HSE_RMEM_SLOTS_NUM);		\
	hse_rmem_memmap[slot];			\
})

#define s32cc__hse_desc_getter(prop) hse_desc_memmap.prop
#define s32cc__hse_regs_getter(prop) hse_regs_memmap.prop

struct hse_memmap {
	uintptr_t paddr;
	uintptr_t vaddr;
	size_t size;
};

extern struct hse_memmap hse_rmem_memmap[];
extern struct hse_memmap hse_desc_memmap;
extern struct hse_memmap hse_regs_memmap;

int hse_set_rmem_space(const void *fdt);
int hse_set_fdt_rmem(const void *fdt, uint32_t slot);

#endif /* FCONF_HSE_GETTER_H */
