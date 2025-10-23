/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/arm/css/scmi.h>
#include <drivers/scmi.h>
#include <scmi-msg/clock.h>
#include "scmi_private.h"

/*
 * API to get the clock rate over SCMI.
 */
int scmi_clock_get_rate(void *p, uint32_t clk_id,
		unsigned long *rate)
{
	struct scmi_clock_rate_get_a2p *payload_args;
	struct scmi_clock_rate_get_p2a *payload_resp;
	scmi_channel_t *ch = (scmi_channel_t *)p;
	unsigned int token = 0;
	mailbox_mem_t *mbx_mem;
	int ret;

	validate_scmi_channel(ch);
	scmi_get_channel(ch);

	mbx_mem = (mailbox_mem_t *)(ch->info->scmi_mbx_mem);
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->len = 4U + sizeof(struct scmi_clock_rate_get_a2p);
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_CLOCK,
			SCMI_CLOCK_RATE_GET, token);

	payload_args = (struct scmi_clock_rate_get_a2p *)
		mbx_mem->payload;
	payload_args->clock_id = clk_id;

	scmi_send_sync_command(ch);

	/* Get the return values */
	SCMI_PAYLOAD_RET_VAL1(mbx_mem->payload, ret);
	assert(token == SCMI_MSG_GET_TOKEN(mbx_mem->msg_header));

	if (ret != SCMI_SUCCESS) {
		ERROR("Failed to get clock rate for clock id: %d, err: %d\n",
				clk_id, ret);
		goto free_scmi_channel;
	}

	payload_resp = (struct scmi_clock_rate_get_p2a *)
		mbx_mem->payload;

	*rate = ((unsigned long)payload_resp->rate[1] << 32) |
		payload_resp->rate[0];

free_scmi_channel:
	scmi_put_channel(ch);
	return ret;
}
