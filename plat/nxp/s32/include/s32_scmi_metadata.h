/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32_SCMI_METADATA_H
#define S32_SCMI_METADATA_H

#include <inttypes.h>

/**
 * Describes the layout of a metadata memory region:
 *  -> timestamps:
 *	TS_AGENT_REQ_TX: the moment when the agent has issued the message
 *	TS_PLAT_REQ_RX: the moment when the platform has started message processing
 *	TS_PLAT_RSP_TX: the moment when the platform filled the reply
 *	TS_AGENT_RSP_RX: the moment when the agent received the reply
 */

enum scmi_log_timestamps {
	TS_AGENT_REQ_TX,
	TS_PLAT_REQ_RX,
	TS_PLAT_RSP_TX,
	TS_AGENT_RSP_RX,
	TS_COUNT
};

struct s32_scmi_metadata {
	uint32_t timestamps[TS_COUNT];
};

#endif /* S32_SCMI_METADATA_H */
