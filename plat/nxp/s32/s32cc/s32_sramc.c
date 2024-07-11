/*
 * Copyright 2020-2022, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch_helpers.h>
#include <errno.h>
#include <inttypes.h>
#include <lib/mmio.h>
#include <platform_def.h>
#include <s32cc_sramc.h>
#include <s32cc_bl_common.h>
#include <common/debug.h>

/* SRAM controller is able to erase 64 bits at once */
#define SRAM_BLOCK	      512
#define SRAM_BLOCK_MASK	 (SRAM_BLOCK - 1)

#define SRAMC_PRAMCR_OFFSET     0x0
#define SRAMC_PRAMCR_INITREQ    1
#define SRAMC_PRAMIAS_OFFSET    0x4
#define SRAMC_PRAMIAE_OFFSET    0x8
#define SRAMC_PRAMSR_OFFSET     0xC
#define SRAMC_PRAMSR_IDONE      1

#define SSRAM_MAX_ADDR	  0x7FF
#define SRAMC_MMAP_SIZE ((size_t)round_up(SRAMC_SIZE, PAGE_SIZE))

int _s32_sram_clr(uintptr_t start, uintptr_t end);

static void a53_sram_init(void *start, size_t len)
{
	memset(start, 0, len);
	if (is_dcache_enabled())
		flush_dcache_range((uintptr_t)start, len);
}

static void clear_unaligned_ends(uintptr_t *start, uintptr_t *end)
{
	uintptr_t leftover;

	if (*start % SRAM_BLOCK) {
		leftover = SRAM_BLOCK - (*start & SRAM_BLOCK_MASK);

		a53_sram_init((void *)*start, round_up(leftover, 8));
		*start += leftover;
	}

	if (*end % SRAM_BLOCK) {
		leftover = *end & SRAM_BLOCK_MASK;

		a53_sram_init((void *)(*end - leftover), round_up(leftover, 8));
		*end -= leftover;
	}
}

static bool in_overlap(uintptr_t s1, uintptr_t e1, uintptr_t s2, uintptr_t e2)
{
	return MAX(s1, s2) <= MIN(e1, e2);
}

static void clear_sramc_range(uintptr_t base, uint32_t start_offset,
			      uint32_t end_offset)
{
	/* Disable the controller */
	mmio_write_32(base + SRAMC_PRAMCR_OFFSET, 0x0);

	/* Max range */
	mmio_write_32(base + SRAMC_PRAMIAS_OFFSET, start_offset);
	mmio_write_32(base + SRAMC_PRAMIAE_OFFSET, end_offset);

	/* Initialization request */
	mmio_write_32(base + SRAMC_PRAMCR_OFFSET, SRAMC_PRAMCR_INITREQ);

	while (!(mmio_read_32(base + SRAMC_PRAMSR_OFFSET) & SRAMC_PRAMSR_IDONE))
		;
	mmio_write_32(base + SRAMC_PRAMSR_OFFSET, SRAMC_PRAMSR_IDONE);
}

static void clear_sram_range(struct sram_ctrl *c, uintptr_t start_addr,
			      uintptr_t end_addr)
{
	uint32_t start_offset, end_offset;
	uintptr_t base = c->base_addr;

	start_addr -= c->min_sram_addr;
	end_addr -= c->min_sram_addr;

	start_offset = c->a53_to_sramc_offset(start_addr);
	end_offset = c->a53_to_sramc_offset(end_addr) - 1;

	if (s32_mmap_dynamic_region(base, SRAMC_MMAP_SIZE, MT_DEVICE | MT_RW,
				    &c->mmap_added))
		panic();

	clear_sramc_range(base, start_offset, end_offset);
}

#ifdef SSRAMC_BASE_ADDR
void s32_ssram_clear(void)
{
	static bool mmap_added;

	if (s32_mmap_dynamic_region(SSRAMC_BASE_ADDR, SRAMC_MMAP_SIZE, MT_DEVICE | MT_RW,
				    &mmap_added))
		panic();

	clear_sramc_range(SSRAMC_BASE_ADDR, 0x0, SSRAM_MAX_ADDR);
}
#endif

int s32_sram_clear(uintptr_t start, uintptr_t end)
{
	struct sram_ctrl *ctrls;
	struct sram_ctrl *c;
	size_t i, n_ctrls;
	uintptr_t s, e;

	if (start == end)
		return 0;

	if (end < start)
		return -EINVAL;

	if (start < S32_SRAM_BASE)
		return -EINVAL;

	if (end > S32_SRAM_END)
		return -EINVAL;

	clear_unaligned_ends(&start, &end);

	s32_get_sramc(&ctrls, &n_ctrls);

	for (i = 0u; i < n_ctrls; i++) {
		c = &ctrls[i];

		if (!in_overlap(start, end, c->min_sram_addr, c->max_sram_addr))
			continue;

		/* Adapt the range to current controller */
		s = MAX(start, (uintptr_t)c->min_sram_addr);
		e = MIN(end, (uintptr_t)c->max_sram_addr);

		clear_sram_range(c, s, e);
	}

	return 0;
}
