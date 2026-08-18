/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arm/css/scmi/scmi_private.h>
#include <lib/utils_def.h>
#include <s32n_scmi_local.h>
#include <s32n_scmi_pinctrl.h>

/*
 * Table indexed by the SCMI protocol ID. Only the protocols that can be served
 * locally have a populated entry; every other index is left zeroed, which means
 * "no local handling" (both callbacks NULL).
 */
static const struct s32n_scmi_local_proto s32n_scmi_local_protos[] = {
	[S32N_SCMI_PROTOCOL_ID_PINCTRL] = {
		.handle_request = s32n_scmi_pinctrl_handle_request,
		.snoop_response = s32n_scmi_pinctrl_snoop_response,
	},
};

static const struct s32n_scmi_local_proto *find_proto(uint32_t msg_header)
{
	uint32_t proto_id = SCMI_MSG_GET_PROTO(msg_header);

	if (proto_id >= ARRAY_SIZE(s32n_scmi_local_protos)) {
		return NULL;
	}

	return &s32n_scmi_local_protos[proto_id];
}

bool s32n_scmi_local_handle_request(struct scmi_shared_mem *mem,
				    size_t mem_size)
{
	const struct s32n_scmi_local_proto *proto;
	uint32_t msg_header = mem->msg_header;

	proto = find_proto(msg_header);
	if (proto == NULL || proto->handle_request == NULL) {
		return false;
	}

	return proto->handle_request(mem, mem_size, msg_header);
}

void s32n_scmi_local_snoop_response(const struct scmi_shared_mem *mem,
				    size_t mem_size)
{
	const struct s32n_scmi_local_proto *proto;
	uint32_t msg_header = mem->msg_header;

	proto = find_proto(msg_header);
	if (proto == NULL || proto->snoop_response == NULL) {
		return;
	}

	proto->snoop_response(mem, mem_size, msg_header);
}
