/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2021, 2023 NXP
 */

#ifndef __DT_BINDINGS_CLOCK_S32G3_H
#define __DT_BINDINGS_CLOCK_S32G3_H

#include <dt-bindings/clock/s32g-clock.h>

#define S32G3_CLK(X)			(S32G_CLK_LAST + (X))
#define S32G_CLK_MC_CGM6_MUX0		S32G3_CLK(0)
#define S32G_CLK_MC_CGM6_MUX1		S32G3_CLK(1)
#define S32G_CLK_MC_CGM6_MUX2		S32G3_CLK(2)
#define S32G_CLK_MC_CGM6_MUX3		S32G3_CLK(3)

#endif //__DT_BINDINGS_CLOCK_S32G3_H
