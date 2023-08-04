/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2021, 2023 NXP
 */
#ifndef S32GEN1_SCMI_RST_H
#define S32GEN1_SCMI_RST_H

#include <stdint.h>
#include <stdbool.h>

#define S32GEN1_NO_MUX_ATTACHED	(0u)

int s32gen1_assert_rgm(void *rgm, bool asserted, uint32_t id);
int s32gen1_reset_periph(uint32_t periph_id, bool assert, uint32_t mux_clk);
int s32gen1_reset_partition(unsigned int part_id, bool assert_not_deassert);

#endif

