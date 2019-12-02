/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32G_NCORE_H
#define S32G_NCORE_H

#include <arch_helpers.h>

#define A53_CLUSTER0_CAIU	(0)
#define A53_CLUSTER1_CAIU	(1)

void ncore_caiu_online(uint32_t caiu);
void ncore_init(void);

#endif /* S32G_NCORE_H */
