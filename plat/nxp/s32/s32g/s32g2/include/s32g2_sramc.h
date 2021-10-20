/*
 * Copyright 2020-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32G2_SRAMC_H
#define S32G2_SRAMC_H

#include <lib/utils_def.h>
#include <s32g_sramc.h>

/* Block ranges */
#define SRAMC0_MIN_ADDR		(0x0)
#define SRAMC0_MAX_ADDR		(0x7FFF)
#define SRAMC1_MIN_ADDR		(SRAMC0_MAX_ADDR + 1)
#define SRAMC1_MAX_ADDR		(0x10000)

#endif

