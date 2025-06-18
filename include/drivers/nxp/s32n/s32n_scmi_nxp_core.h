// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2025 NXP
 */

#ifndef S32N_SCMI_NXP_CORE_H
#define S32N_SCMI_NXP_CORE_H

#include <stdint.h>

/*
 * Identifiers of the NXP Core Protocol commands
 */
enum scmi_nxp_core_command_id {
	SCMI_NXP_CORE_CONFIGURE_CORE = 0x3,
};

/* Protocol attributes */
#define SCMI_NXP_CORE_PROTO_ATTR_MSG_LEN	U(4)
#define SCMI_NXP_CORE_PROTO_ATTR_RESP_LEN	U(12)
#define SCMI_NXP_CORE_SOC_CORE_COUNT_MASK	GENMASK_32(15, 0)

/*
 * NXP Core Configure Core
 */
struct scmi_nxp_core_configure_core_a2p {
	uint32_t core_id;
	uint32_t low_entrypoint;
	uint32_t high_entrypoint;
};

#endif  /* S32N_SCMI_NXP_CORE_H */
