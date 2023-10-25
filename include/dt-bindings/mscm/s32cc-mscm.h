/* SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause */
/*
 * Copyright 2023 NXP
 */

#ifndef DT_BINDINGS_MSCM_S32CC_H
#define DT_BINDINGS_MSCM_S32CC_H

/* Core Processor Numbers */
#define A53_0_CPN       (0)
#define A53_1_CPN       (1)
#define A53_2_CPN       (2)
#define A53_3_CPN       (3)
#define M7_0_CPN        (4)
#define M7_1_CPN        (5)
#define M7_2_CPN        (6)

#define MSCM_CPN_MAX    M7_2_CPN

/* MSCM core-to-core IRQS */
#define MSCM_C2C_IRQ_0      (0)
#define MSCM_C2C_IRQ_1      (1)
#define MSCM_C2C_IRQ_2      (2)

#define MSCM_C2C_IRQ_MAX    MSCM_C2C_IRQ_2

#endif /* DT_BINDINGS_MSCM_S32CC_H */

