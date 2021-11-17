/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2021 NXP
 */
#ifndef S32GEN1_SCMI_RST_H
#define S32GEN1_SCMI_RST_H

#include <stdint.h>
#include <stdbool.h>

int s32gen1_reset_periph(uint32_t periph_id, bool assert);
int s32gen1_reset_partition(unsigned int part_id, bool assert_not_deassert);

#endif

