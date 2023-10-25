/* SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause */
/*
 * Copyright 2023 NXP
 */

#ifndef DT_BINDINGS_MSCM_S32G3_H
#define DT_BINDINGS_MSCM_S32G3_H

#include <dt-bindings/mscm/s32cc-mscm.h>

/* Core Processor Numbers */
#undef A53_2_CPN
#undef A53_3_CPN

#define M7_3_CPN        (7)
#define A53_2_CPN       (8)
#define A53_3_CPN       (9)
#define A53_4_CPN       (2)
#define A53_5_CPN       (3)
#define A53_6_CPN       (10)
#define A53_7_CPN       (11)

#undef MSCM_CPN_MAX
#define MSCM_CPN_MAX    A53_7_CPN

/* MSCM core-to-core IRQS */
#define MSCM_C2C_IRQ_3      (3)
#define MSCM_C2C_IRQ_4      (4)
#define MSCM_C2C_IRQ_5      (5)
#define MSCM_C2C_IRQ_6      (6)
#define MSCM_C2C_IRQ_7      (7)
#define MSCM_C2C_IRQ_8      (8)
#define MSCM_C2C_IRQ_9      (9)
#define MSCM_C2C_IRQ_10     (10)
#define MSCM_C2C_IRQ_11     (11)

#undef MSCM_C2C_IRQ_MAX
#define MSCM_C2C_IRQ_MAX   MSCM_C2C_IRQ_11

#endif /* DT_BINDINGS_MSCM_S32G3_H */

