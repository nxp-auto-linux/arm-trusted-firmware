/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/mmio.h>
#include <errno.h>
#include <platform_def.h>
#include "s32g_sramc.h"

int _s32g_sram_clr(uintptr_t start, uintptr_t end);

void s32g_ssram_clear(void)
{
	uintptr_t base = SSRAMC_BASE_ADDR;

	/* Disable the controller */
	mmio_write_32(base + SRAMC_PRAMCR_OFFSET, 0x0);

	/* Max range */
	mmio_write_32(base + SRAMC_PRAMIAS_OFFSET, 0x0);
	mmio_write_32(base + SRAMC_PRAMIAE_OFFSET, SSRAM_MAX_ADDR);

	/* Initialization request */
	mmio_write_32(base + SRAMC_PRAMCR_OFFSET, SRAMC_PRAMCR_INITREQ);

	while (!(mmio_read_32(base + SRAMC_PRAMSR_OFFSET) & SRAMC_PRAMSR_IDONE))
		;
	mmio_write_32(base + SRAMC_PRAMSR_OFFSET, SRAMC_PRAMSR_IDONE);
}

int s32g_sram_clear(uintptr_t start, uintptr_t end)
{
	uintptr_t low, high;
	int ret;

	if (start == end)
		return 0;

	if (end < start)
		return -EINVAL;

	start = round_down(start, SRAM_BLOCK);
	end = round_up(end, SRAM_BLOCK);

	if (start < S32G_SRAM_BASE)
		return -EINVAL;

	if (end > S32G_SRAM_END)
		return -EINVAL;

	/* Bus addresses */
	low = (start - S32G_SRAM_BASE);
	high = (end - S32G_SRAM_BASE);

	ret = _s32g_sram_clr(low, high);
	if (ret)
		return ret;

	return 0;
}
