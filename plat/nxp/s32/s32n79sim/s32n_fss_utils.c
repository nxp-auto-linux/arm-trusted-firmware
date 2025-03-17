/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arm/css/scmi/scmi_private.h>
#include <drivers/arm/css/scmi.h>
#include <scmi-msg/nxp-core.h>

#include <s32n_fss_scmi.h>

#define ADDR_MASK		GENMASK(31, 0)
#define ADDR_SHIFT		U(32)

int nxp_core_configure_core(void *p, uint32_t core_id, uintptr_t entrypoint)
{
	struct scmi_nxp_core_configure_core_a2p *payload_args;
	scmi_channel_t *ch = (scmi_channel_t *)p;
	unsigned int token = 0;
	mailbox_mem_t *mbx_mem;
	int ret;

	validate_scmi_channel(ch);

	scmi_get_channel(ch);

	mbx_mem = (mailbox_mem_t *)(ch->info->scmi_mbx_mem);
	mbx_mem->res_a = 0U;
	mbx_mem->res_b = 0UL;
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->len = 4U + sizeof(struct scmi_nxp_core_configure_core_a2p);
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_NXP_CORE,
					      SCMI_NXP_CORE_CONFIGURE_CORE,
					      token);

	payload_args = (struct scmi_nxp_core_configure_core_a2p *)mbx_mem->payload;
	payload_args->core_id = core_id;
	payload_args->low_entrypoint = entrypoint & ADDR_MASK;
	payload_args->high_entrypoint = entrypoint >> ADDR_SHIFT;

	scmi_send_sync_command(ch);

	/* Get the return values */
	SCMI_PAYLOAD_RET_VAL1(mbx_mem->payload, ret);
	assert(token == SCMI_MSG_GET_TOKEN(mbx_mem->msg_header));

	scmi_put_channel(ch);

	return ret;
}
