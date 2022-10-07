/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2022 NXP
 */
#ifndef __DT_BINDINGS_SCMI_PD_S32GEN1_H
#define __DT_BINDINGS_SCMI_PD_S32GEN1_H

/*
 * Generic way of accessing a core power domain
 * X can vary from 0 to S32GEN1_SCMI_PD_A53_MAX (included)
 */
#define S32GEN1_SCMI_PD_A53(X)		(X)
/* Power domain id to core id */
#define S32GEN1_SCMI_PD_ID_TO_A53(X)	(X)

/*
 * Power domain tree on S32CC platforms
 *
 *         ------------SYSTOP----------               - Level 1
 *         |   |     |     |    |     |
 *         |   |     |     |    |     |
 *      A53_0 A53_1 A53_2 A53_3 ... A53_MAX           - Level 0
 *
 * SCMI power state parameter bit field encoding for S32CC platforms.
 *
 * 31  20 19       16                       7        4 3         0
 * +-------------------------------------------------------------+
 * |     | Max level |                     |  Level 1 |  Level 0 |
 * |     |           |                     |   state  |   state  |
 * +-------------------------------------------------------------+
 *
 * `Max level` encodes the highest level that has a valid power state
 * encoded in the power state.
 */

/* List of power domains */
/* Cores */
#define S32GEN1_SCMI_PD_A53_0		(0u)
#define S32GEN1_SCMI_PD_A53_1		(1u)
#define S32GEN1_SCMI_PD_A53_2		(2u)
#define S32GEN1_SCMI_PD_A53_3		(3u)
#define S32GEN1_SCMI_PD_A53_MAX		(S32GEN1_SCMI_PD_A53_3)

/* System level power domain */
#define S32GEN1_SCMI_PD_SYSTOP		(S32GEN1_SCMI_PD_A53_MAX + 1)
#define S32GEN1_SCMI_MAX_PD		(S32GEN1_SCMI_PD_SYSTOP + 1)

/* Level states */
#define S32GEN1_SCMI_PD_OFF		(0u)
#define S32GEN1_SCMI_PD_ON		(1u)
#define S32GEN1_SCMI_PD_SLEEP		(2u)
#define S32GEN1_SCMI_PD_MAX_STATE	(3u)

#define S32GEN1_SCMI_PD_LEVEL0_SHIFT	(0u)
#define S32GEN1_SCMI_PD_LEVEL1_SHIFT	(4u)
#define S32GEN1_SCMI_PD_MAX_LEVEL_SHIFT	(16u)
#define S32GEN1_SCMI_PD_LEVEL_MASK	(0xfu)

#define S32GEN1_SCMI_PD_GET_LEVEL_STATE(PSTATE, LEVEL_SH) \
	(((PSTATE) >> (LEVEL_SH)) & S32GEN1_SCMI_PD_LEVEL_MASK)

#define S32GEN1_SCMI_PD_GET_LEVEL0_STATE(PSTATE) \
	S32GEN1_SCMI_PD_GET_LEVEL_STATE(PSTATE, S32GEN1_SCMI_PD_LEVEL0_SHIFT)

#define S32GEN1_SCMI_PD_GET_LEVEL1_STATE(PSTATE) \
	S32GEN1_SCMI_PD_GET_LEVEL_STATE(PSTATE, S32GEN1_SCMI_PD_LEVEL1_SHIFT)

#define S32GEN1_SCMI_PD_SET_LEVEL_STATE(LEVEL_STATE, LEVEL_SH) \
	(((LEVEL_STATE) & S32GEN1_SCMI_PD_LEVEL_MASK) << (LEVEL_SH))

#define S32GEN1_SCMI_PD_SET_LEVEL0_STATE(LEVEL_STATE) \
	S32GEN1_SCMI_PD_SET_LEVEL_STATE(LEVEL_STATE, \
					S32GEN1_SCMI_PD_LEVEL0_SHIFT)

#define S32GEN1_SCMI_PD_SET_LEVEL1_STATE(LEVEL_STATE) \
	S32GEN1_SCMI_PD_SET_LEVEL_STATE(LEVEL_STATE, \
					S32GEN1_SCMI_PD_LEVEL1_SHIFT)

#define S32GEN1_SCMI_PD_SET_MAX_LEVEL_STATE(LEVEL_STATE) \
	S32GEN1_SCMI_PD_SET_LEVEL_STATE(LEVEL_STATE, \
					S32GEN1_SCMI_PD_MAX_LEVEL_SHIFT)

#endif
