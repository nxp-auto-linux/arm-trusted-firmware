/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32CC_SCMI_METADATA_H
#define S32CC_SCMI_METADATA_H

#include <inttypes.h>

/**
 * Describes the layout of a metadata memory region:
 *  -> command timestamps:
 *	TS_AGENT_REQ_TX: the moment when the agent has issued the message
 *	TS_PLAT_REQ_RX: the moment when the platform has started message processing
 *	TS_PLAT_RSP_TX: the moment when the platform filled the reply
 *	TS_AGENT_RSP_RX: the moment when the agent received the reply
 *  -> notification timestamps:
 *  TS_PLAT_NOTIF_TX: the moment when the platform filled the notification payload
 *  TS_AGENT_NOTIF_RX: the moment when the agent has started notification processing
 *  TS_AGENT_ACK_RX: the moment when the agent received notification ack
 */

enum scmi_log_cmd_timestamps {
	TS_AGENT_REQ_TX,
	TS_PLAT_REQ_RX,
	TS_PLAT_RSP_TX,
	TS_AGENT_RSP_RX,
	TS_COUNT
};

enum scmi_log_notif_timestamps {
	TS_PLAT_NOTIF_TX,
	TS_AGENT_NOTIF_RX,
	TS_AGENT_ACK_RX,
	TS_NOTIF_COUNT,
};

struct s32_scmi_metadata {
	uint32_t timestamps[TS_COUNT];
};

#endif /* S32CC_SCMI_METADATA_H */
