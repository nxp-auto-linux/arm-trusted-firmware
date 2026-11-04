/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2026 NXP
 */
#ifndef S32CC_CLK_INDEX_H
#define S32CC_CLK_INDEX_H

#include <dt-bindings/clock/s32cc-clock.h>
#include <lib/cassert.h>
#include <stdint.h>

/*
 * Clock index of the S32CC clock driver, private to drivers/nxp/s32/clk: the
 * S32CC_CC_CLK() IDs come first, then the S32CC_PERIPH_CLK() IDs, then the
 * platform clock IDs. Each range is sized from the clock bindings.
 */
#define S32CC_BASE_CLK_SLOTS	((uint32_t)S32CC_CLK_NUM_PROVIDERS + \
				 (uint32_t)S32CC_PERIPH_CLK_NUM)
#define S32CC_PLAT_CLK_SLOTS	((uint32_t)S32CC_PLAT_CLK_NUM)
#define S32CC_CLK_SLOTS		(S32CC_BASE_CLK_SLOTS + S32CC_PLAT_CLK_SLOTS)

/* The base slots must not run into the platform clock IDs */
CASSERT((S32CC_CLK_ID_BASE + S32CC_CLK_NUM_PROVIDERS + S32CC_PERIPH_CLK_NUM) <=
	S32CC_PLAT_CLK_ID_BASE, assert_s32cc_base_clk_ids_overlap_plat_clk_ids);

/* Position of a clock ID in the clock index */
int get_clock_idx(uint32_t id, uint32_t *idx);

#endif /* S32CC_CLK_INDEX_H */
