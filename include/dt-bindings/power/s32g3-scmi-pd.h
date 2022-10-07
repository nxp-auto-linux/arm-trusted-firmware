/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2022 NXP
 */
#ifndef __DT_BINDINGS_SCMI_PD_S32G3_H
#define __DT_BINDINGS_SCMI_PD_S32G3_H

#include "s32gen1-scmi-pd.h"

/* List of power domains */
/* Cores */
#define S32G3_SCMI_PD_A53_4	(4u)
#define S32G3_SCMI_PD_A53_5	(5u)
#define S32G3_SCMI_PD_A53_6	(6u)
#define S32G3_SCMI_PD_A53_7	(7u)

#undef S32GEN1_SCMI_PD_A53_MAX
#define S32GEN1_SCMI_PD_A53_MAX	(S32G3_SCMI_PD_A53_7)

#endif
