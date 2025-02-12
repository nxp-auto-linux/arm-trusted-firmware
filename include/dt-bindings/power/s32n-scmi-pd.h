/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2025 NXP
 */
#ifndef __DT_BINDINGS_SCMI_PD_S32N_H
#define __DT_BINDINGS_SCMI_PD_S32N_H

/*
 * List of APS Cores Power Domain IDs,
 * as defined in the FSS Resource Manager header files.
 */
#define S32N_APS0_CORE0		(0x1d6u)
#define S32N_APS0_CORE1		(0x1d7u)
#define S32N_APS1_CORE0		(0x1efu)
#define S32N_APS1_CORE1		(0x1f0u)
#define S32N_APS2_CORE0		(0x208u)
#define S32N_APS2_CORE1		(0x209u)
#define S32N_APS3_CORE0		(0x221u)
#define S32N_APS3_CORE1		(0x222u)

/*
 * SCMI power state parameter bit field encoding for S32N platforms.
 *
 * 31     30                                 7          4 3         0
 * +----------------------------------------------------------------+
 * |   | State |                             |  Level 1  |  Level 0 |
 * |   | Type  |                             |   state   |   state  |
 * |   |       |                             | (Cluster) |  (Core)  |
 * +----------------------------------------------------------------+
 *
 * The power state indicates the deepest tolerated state for a core
 * and its parent nodes.
 */

/* Level states */
#define S32N_SCMI_PD_LEVEL_ON		(0u)
#define S32N_SCMI_PD_LEVEL_RET      (1u)
#define S32N_SCMI_PD_LEVEL_SLEEP	(3u)
#define S32N_SCMI_PD_LEVEL_OFF		(7u)

/* State Type */
#define S32N_SCMI_PD_STATE_STANDBY	(0u)
#define S32N_SCMI_PD_STATE_PWR_DWN	(1u)

#define S32N_SCMI_PD_LEVEL0_SHIFT		(0u)
#define S32N_SCMI_PD_LEVEL1_SHIFT		(4u)
#define S32N_SCMI_PD_STATE_TYPE_SHIFT	(30u)

#define S32N_SCMI_PD_LEVEL_MASK			GENMASK_32(3, 0)
#define S32N_SCMI_PD_STATE_TYPE_MASK	(1u)

#define S32N_SCMI_PD_SET_LEVEL_STATE(LEVEL_STATE, LEVEL_SH, LEVEL_MASK) \
	(((LEVEL_STATE) & LEVEL_MASK) << (LEVEL_SH))

#define S32N_SCMI_PD_SET_LEVEL0_STATE(LEVEL_STATE) \
	S32N_SCMI_PD_SET_LEVEL_STATE(LEVEL_STATE, \
		S32N_SCMI_PD_LEVEL0_SHIFT, S32N_SCMI_PD_LEVEL_MASK)

#define S32N_SCMI_PD_SET_LEVEL1_STATE(LEVEL_STATE) \
	S32N_SCMI_PD_SET_LEVEL_STATE(LEVEL_STATE, \
		S32N_SCMI_PD_LEVEL1_SHIFT, S32N_SCMI_PD_LEVEL_MASK)

#define S32N_SCMI_PD_SET_STATE_TYPE(LEVEL_STATE) \
	S32N_SCMI_PD_SET_LEVEL_STATE(LEVEL_STATE, \
		S32N_SCMI_PD_STATE_TYPE_SHIFT, S32N_SCMI_PD_STATE_TYPE_MASK)

#endif

