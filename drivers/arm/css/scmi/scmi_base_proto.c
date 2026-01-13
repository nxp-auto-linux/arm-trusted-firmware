/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/arm/css/scmi.h>
#include <drivers/scmi.h>
#include <scmi-msg/base.h>
#include "scmi_private.h"

/*
 * API to subscribe for Base protocol errors over SCMI
 */
int scmi_subscribe_base_error_notify(void *p, uint32_t notify_enable)
{
	mailbox_mem_t *mbx_mem;
	unsigned int token = 0;
	int ret;
	scmi_channel_t *ch = (scmi_channel_t *)p;

	validate_scmi_channel(ch);
	scmi_get_channel(ch);

	mbx_mem = (mailbox_mem_t *)(ch->info->scmi_mbx_mem);
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_BASE,
					      SCMI_BASE_NOTIFY_ERRORS, token);
	mbx_mem->len = SCMI_BASE_NOTIFY_ERRORS_MSG_LEN;
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	SCMI_PAYLOAD_ARG1(mbx_mem->payload, notify_enable);

	scmi_send_sync_command(ch);

	/* Get the return values */
	SCMI_PAYLOAD_RET_VAL1(mbx_mem->payload, ret);
	assert(mbx_mem->len == SCMI_BASE_NOTIFY_ERRORS_RESP_LEN);
	assert(token == SCMI_MSG_GET_TOKEN(mbx_mem->msg_header));

	scmi_put_channel(ch);

	return ret;
}
