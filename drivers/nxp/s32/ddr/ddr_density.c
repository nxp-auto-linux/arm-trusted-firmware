// SPDX-License-Identifier: (GPL-2.0+ OR BSD-3-Clause)
/*
 * Copyright 2022 NXP
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <ddr/ddr_density.h>
#include "ddr_utils.h"

#define OFFSET_DDRC_START_ADDRMAP	0x21C

#define NO_ADDRMAP_REGS			8

/* ECC */
#define ECC_MODE_MASK	0x00000007
#define ECC_MODE_SHIFT	0x00000000
#define ECC_DISABLE	0x00000000
#define ECC_FLAG_MASK	0x00000001

/* Hardware limitation */
#define S32GEN1_DDR_MAX_NO_PAGES	5

#define S32GEN1_SRAM_BASE		0x34000000ULL
#define S32GEN1_DRAM_STD_ADDR	0x80000000ULL
#define S32GEN1_DRAM_EXT_ADDR	0x800000000ULL

#define ECC_FLAG_SHIFT	0
#define ECC_ON		1
#define ECC_OFF		0

struct s32_ddr_region {
	unsigned long flags; // e.g. ECC ON/OFF
	unsigned long address; // start address
	unsigned long size; // memory region size - total density
};

enum addr_map_masks {
	ADDRMAP7 = 0x00000f0f,
	ADDRMAP6 = 0x0f0f0f0f,
	ADDRMAP5 = 0x0f0f0f00,
	ADDRMAP4 = 0x00001f1f,
	ADDRMAP3 = 0x1f1f1f1f,
	ADDRMAP2 = 0x0f0f1f0f,
	ADDRMAP1 = 0x003f3f3f,
	ADDRMAP0 = 0x0000001f,
};

enum addr_map_internal_base {
	ADDRMAP7_BASE = 0x00001716,
	ADDRMAP6_BASE = 0x15141312,
	ADDRMAP5_BASE = 0x11060706,
	ADDRMAP4_BASE = 0x00000b0a,
	ADDRMAP3_BASE = 0x09080706,
	ADDRMAP2_BASE = 0x05040302,
	ADDRMAP1_BASE = 0x00040302,
	ADDRMAP0_BASE = 0x00000006,
};

enum addr_map_shift {
	ADDRMAP7_SHIFT = 0x00000800,
	ADDRMAP6_SHIFT = 0x18100800,
	ADDRMAP5_SHIFT = 0x18100800,
	ADDRMAP4_SHIFT = 0x00000800,
	ADDRMAP3_SHIFT = 0x18100800,
	ADDRMAP2_SHIFT = 0x18100800,
	ADDRMAP1_SHIFT = 0x00100800,
	ADDRMAP0_SHIFT = 0x00000000,
};

static const enum addr_map_masks addr_map_masks_map[] = {
	ADDRMAP7, ADDRMAP6, ADDRMAP5, ADDRMAP4,
	ADDRMAP3, ADDRMAP2, ADDRMAP1, ADDRMAP0
};

static const enum addr_map_shift addr_map_shift_map[] = {
	ADDRMAP7_SHIFT, ADDRMAP6_SHIFT, ADDRMAP5_SHIFT,
	ADDRMAP4_SHIFT, ADDRMAP3_SHIFT, ADDRMAP2_SHIFT,
	ADDRMAP1_SHIFT, ADDRMAP0_SHIFT
};

static const enum addr_map_internal_base addr_map_internal_base_map[] = {
	ADDRMAP7_BASE, ADDRMAP6_BASE, ADDRMAP5_BASE, ADDRMAP4_BASE,
	ADDRMAP3_BASE, ADDRMAP2_BASE, ADDRMAP1_BASE, ADDRMAP0_BASE
};

static inline bool get_intersection(unsigned long s1, unsigned long e1,
				    unsigned long s2, unsigned long e2,
				    unsigned long *s3, unsigned long *e3)
{
	if (s2 > e1 || s1 > e2)
		return false;

	*s3 = MAX(s1, s2);
	*e3 = MIN(e1, e2);

	return true;
}

static bool is_ext_addr(unsigned long addr)
{
	return addr >= S32GEN1_DRAM_EXT_ADDR;
}

static unsigned long to_ext_addr(unsigned long addr)
{
	return addr - S32GEN1_DRAM_STD_ADDR + S32GEN1_DRAM_EXT_ADDR;
}

static unsigned long to_std_addr(unsigned long addr)
{
	return addr + S32GEN1_DRAM_STD_ADDR - S32GEN1_DRAM_EXT_ADDR;
}

static inline unsigned int get_byte(unsigned int val, unsigned int nr)
{
	return (val >> (8 * nr)) & 0xff;
}

static void s32gen1_get_ddr_regions(struct s32_ddr_region
			     pages[S32GEN1_DDR_MAX_NO_PAGES],
			     unsigned int *active_pages)
{
	unsigned int i, sh, mk;
	unsigned int *reg = (unsigned int *)(DDRC_BASE_ADDR +
			OFFSET_DDRC_START_ADDRMAP);
	unsigned int idx, tmp, reg_val, max_hif = 0;
	struct s32_ddr_region curr_page = {
		.address = 0x800000000,
		/* Reset value */
		.flags = 0x0,
	};

	reg_val = mmio_read_32(DDRC_BASE_ADDR + OFFSET_DDRC_ECCCFG0);
	reg_val = (reg_val & ECC_MODE_MASK) >> ECC_MODE_SHIFT;

	if (reg_val != ECC_DISABLE)
		curr_page.flags |= ECC_ON;

	/**
	 * Calculate the size.
	 * Use HIF address bit number to determine total density
	 */
	for (idx = 0; idx < NO_ADDRMAP_REGS ; idx++, reg--) {
		/* We start with highest reg */
		reg_val = mmio_read_32((uintptr_t) reg);

		if ((reg_val & addr_map_masks_map[idx]) ==
		    addr_map_masks_map[idx])
			continue;

		for (i = 0; i < 4; i++) {
			sh = get_byte(addr_map_shift_map[idx], i);
			mk = get_byte(addr_map_masks_map[idx], i);
			/**
			 * If is equals to mask value it means it is disabled so
			 * we don't need to process it
			 */
			tmp = ((reg_val >> sh) & mk);
			if (tmp != mk) {
				tmp += get_byte(addr_map_internal_base_map[idx],
						i);
				max_hif = MAX(tmp, max_hif);
			}
		}
	}

	max_hif++;
	curr_page.size = (unsigned int) (1 << max_hif);

	/**
	 * Convert to AXI
	 * See details in System Address to Physical Address Mapping
	 */
	curr_page.size <<= 2;

	/**
	 * If ECC is used we need to exclude the ECC region
	 * ECC is always the last 1/8 of the memory
	 */
	if (curr_page.flags & ECC_FLAG_MASK)
		curr_page.size = (curr_page.size * 7) / 8;

	*active_pages = 1;

	pages[0] = curr_page;
}

void s32gen1_exclude_ecc(unsigned long *start, unsigned long *size)
{
	static struct s32_ddr_region pages[S32GEN1_DDR_MAX_NO_PAGES];
	static unsigned int active_pages;
	static bool init_pages;
	unsigned long pg_start, pg_end;
	unsigned long r_start, r_end;
	bool std_map;
	unsigned int j;

	if (!init_pages) {
		s32gen1_get_ddr_regions(pages, &active_pages);
		init_pages = true;
	}

	/* Skip SRAM */
	if (*start == S32GEN1_SRAM_BASE)
		return;

	/* Use extended addresses */
	if (!is_ext_addr(*start)) {
		*start = to_ext_addr(*start);
		std_map = true;
	} else {
		std_map = false;
	}

	for (j = 0; j < active_pages; j++) {
		pg_start = pages[j].address;
		pg_end = pg_start + pages[j].size;

		if (!get_intersection(*start, *start + *size,
				      pg_start, pg_end,
				      &r_start, &r_end))
			continue;

		*start = r_start;
		*size = r_end - r_start;
	}

	if (std_map)
		*start = to_std_addr(*start);
}
