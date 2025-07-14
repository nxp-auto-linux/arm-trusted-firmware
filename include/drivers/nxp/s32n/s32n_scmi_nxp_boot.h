// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2025 NXP
 */

#ifndef S32N_SCMI_NXP_BOOT_H
#define S32N_SCMI_NXP_BOOT_H

#include <lib/utils_def.h>

#define REQ_COHORT_ID_MASK		GENMASK_32(31, 16)
#define REQ_COHORT_ID_SHIFT		(16)
#define SW_ENTITY_MASK			GENMASK_32(15, 8)
#define SW_ENTITY_SHIFT			(8)
#define RESET_TYPE_MASK			GENMASK_32(7, 0)

/*
 * Identifiers of the NXP Boot Protocol commands
 */
enum scmi_nxp_boot_command_id {
	SCMI_NXP_GET_RESET_REASON_INFO = 0xD,
};

/*
 * NXP Get Reset Reason SW entity types.
 */
enum scmi_nxp_boot_reset_sw_entity_type {
	SWENTITY_USER_CID = 0x1,
	SWENTITY_BTM_INIT = 0x11,
	SWENTITY_SAFETY_MANAGER = 0x21,
	SWENTITY_SECURITY_MANAGER = 0x31,
};

/*
 * NXP Get Reset Reason types.
 */
enum scmi_nxp_boot_reset_reason_type {
	RESET_TYPE_UNKNOWN = 0x0,
	RESET_TYPE_BOOT = 0x1,
	RESET_TYPE_COLDRESET = 0x2,
	RESET_TYPE_WARMRESET = 0x3,
	RESET_TYPE_WAKEUP = 0x4,
	RESET_TYPE_WARMRESET_ESCALATED = 0x13, /* Escalated to Cold Reset */
	RESET_TYPE_WAKEUP_ESCALATED = 0x14, /* Escalated to Cold Reset */
};

struct scmi_nxp_boot_get_reset_reason_info_a2p {
	uint32_t cohort_id;
} __packed;

struct scmi_nxp_boot_get_reset_reason_info_p2a {
	int32_t status;
	uint32_t reset_reason;
} __packed;

#endif /* S32N_SCMI_NXP_BOOT_H */
