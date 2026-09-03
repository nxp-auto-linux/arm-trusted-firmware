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
 * locally have a populated entry pointing at their per-message ops table; every
 * other index is left zeroed, which means "no local handling".
 */
static const struct s32n_scmi_msg_table *const s32n_scmi_local_protos[] = {
	[S32N_SCMI_PROTOCOL_ID_PINCTRL] = &s32n_scmi_pinctrl_msg_table,
};

static const struct s32n_scmi_msg_ops *find_ops(uint32_t msg_header)
{
	uint32_t proto_id = SCMI_MSG_GET_PROTO(msg_header);
	uint32_t msg_id = SCMI_MSG_GET_MSG_ID(msg_header);
	const struct s32n_scmi_msg_table *table;

	if (proto_id >= ARRAY_SIZE(s32n_scmi_local_protos)) {
		return NULL;
	}

	table = s32n_scmi_local_protos[proto_id];
	if (table == NULL || msg_id >= table->nr_ops) {
		return NULL;
	}

	return &table->ops[msg_id];
}

bool s32n_scmi_local_handle_request(struct scmi_shared_mem *mem,
				    size_t mem_size)
{
	uint32_t msg_header = mem->msg_header;
	const struct s32n_scmi_msg_ops *ops = find_ops(msg_header);

	if (ops == NULL || ops->handle_request == NULL) {
		return false;
	}

	return ops->handle_request(mem, mem_size);
}

void s32n_scmi_local_snoop_response(const struct scmi_shared_mem *mem,
				    size_t mem_size)
{
	uint32_t msg_header = mem->msg_header;
	const struct s32n_scmi_msg_ops *ops = find_ops(msg_header);

	if (ops == NULL || ops->snoop_response == NULL) {
		return;
	}

	ops->snoop_response(mem, mem_size);
}
