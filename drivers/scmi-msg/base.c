// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2015-2019, Arm Limited and Contributors. All rights reserved.
 * Copyright (c) 2019-2022, Linaro Limited
 * Copyright 2020, 2025 NXP
 */
#include <assert.h>
#include <string.h>

#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>
#include <lib/utils.h>
#include <lib/utils_def.h>

#include "common.h"

#define SCMI_RESET_FLAG_MASK 0x1

static bool message_id_is_supported(unsigned int message_id);

#pragma weak plat_scmi_reset_agent
#pragma weak plat_scmi_agent_count
#pragma weak plat_scmi_agent_get_name

int32_t plat_scmi_reset_agent(unsigned int agent_id)
{
	return 0;
}

size_t plat_scmi_agent_count(void)
{
	return 0U;
}

const char *plat_scmi_agent_get_name(unsigned int agent_id __unused)
{
	return NULL;
}

static void report_version(struct scmi_msg *msg)
{
	struct scmi_protocol_version_p2a return_values = {
		.status = SCMI_SUCCESS,
		.version = SCMI_PROTOCOL_VERSION_BASE,
	};

	if (msg->in_size != 0U) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static void report_attributes(struct scmi_msg *msg)
{
	size_t protocol_count = plat_scmi_protocol_count();
	size_t agent_count = plat_scmi_agent_count();
	struct scmi_protocol_attributes_p2a return_values = {
		.status = SCMI_SUCCESS,
		.attributes = SCMI_BASE_PROTOCOL_ATTRIBUTES(protocol_count,
						    agent_count),
	};

	if (msg->in_size != 0U) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static void base_notify_errors(struct scmi_msg *msg)
{
	uint32_t notify_enable;

	if (msg->in_size != sizeof(uint32_t)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	notify_enable = *(uint32_t *)msg->in;

	if (notify_enable != 0 && notify_enable != 1) {
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	scmi_status_response(msg, SCMI_SUCCESS);
}

static void report_message_attributes(struct scmi_msg *msg)
{
	struct scmi_protocol_message_attributes_a2p *in_args = (void *)msg->in;
	struct scmi_protocol_message_attributes_p2a return_values = {
		.status = SCMI_SUCCESS,
		/* For this protocol, attributes shall be zero */
		.attributes = 0U,
	};

	if (msg->in_size != sizeof(*in_args)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	if (!message_id_is_supported(in_args->message_id)) {
		scmi_status_response(msg, SCMI_NOT_FOUND);
		return;
	}

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static void discover_vendor(struct scmi_msg *msg)
{
	const char *name = plat_scmi_vendor_name();
	struct scmi_base_discover_vendor_p2a return_values = {
		.status = SCMI_SUCCESS,
	};

	if (msg->in_size != 0U) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	COPY_NAME_IDENTIFIER(return_values.vendor_identifier, name);

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static void discover_sub_vendor(struct scmi_msg *msg)
{
	const char *name = plat_scmi_sub_vendor_name();
	struct scmi_base_discover_sub_vendor_p2a return_values = {
		.status = SCMI_SUCCESS,
	};

	if (msg->in_size != 0U) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	COPY_NAME_IDENTIFIER(return_values.sub_vendor_identifier, name);

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static void discover_implementation_version(struct scmi_msg *msg)
{
	struct scmi_protocol_version_p2a return_values = {
		.status = SCMI_SUCCESS,
		.version = SCMI_IMPL_VERSION,
	};

	if (msg->in_size != 0U) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static unsigned int count_protocols_in_list(const uint8_t *protocol_list)
{
	unsigned int count = 0U;

	if (protocol_list != NULL) {
		while (protocol_list[count] != 0U) {
			count++;
		}
	}

	return count;
}

static void discover_list_protocols(struct scmi_msg *msg)
{
	const struct scmi_base_discover_list_protocols_a2p *a2p = NULL;
	struct scmi_base_discover_list_protocols_p2a p2a = {
		.status = SCMI_SUCCESS,
	};
	const uint8_t *list = NULL;
	unsigned int count = 0U;

	if (msg->in_size != sizeof(*a2p)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	a2p = (void *)msg->in;

	list = plat_scmi_protocol_list(msg->agent_id);
	count = count_protocols_in_list(list);

	if (a2p->skip >= count) {
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	if (count > a2p->skip) {
		count = MIN((uint32_t)(count - a2p->skip),
			    (uint32_t)(msg->out_size - sizeof(p2a)));
	} else {
		count = 0U;
	}

	p2a.num_protocols = count;

	memcpy(msg->out, &p2a, sizeof(p2a));
	memcpy(msg->out + sizeof(p2a), list + a2p->skip, count);
	msg->out_size_out = sizeof(p2a) + round_up(count, sizeof(uint32_t));
}

static void discover_agent(struct scmi_msg *msg)
{
	const struct scmi_base_discover_agent_a2p *a2p = NULL;
	struct scmi_base_discover_agent_p2a p2a = {
		.status = SCMI_SUCCESS,
	};
	const char *name = NULL;
	uint32_t agent_id;

	if (msg->in_size != sizeof(*a2p)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	a2p = (void *)msg->in;
	agent_id = a2p->agent_id;

	if (agent_id == 0xFFFFFFFFU) {
		agent_id = msg->agent_id;
	}

	if (agent_id >= plat_scmi_agent_count()) {
		scmi_status_response(msg, SCMI_NOT_FOUND);
		return;
	}

	name = plat_scmi_agent_get_name(agent_id);
	if (name == NULL) {
		scmi_status_response(msg, SCMI_NOT_FOUND);
		return;
	}

	p2a.agent_id = agent_id;
	COPY_NAME_IDENTIFIER(p2a.name, name);

	scmi_write_response(msg, &p2a, sizeof(p2a));
}

static void reset_agent_config(struct scmi_msg *msg)
{
	const struct scmi_base_reset_agent_a2p *a2p = NULL;
	struct scmi_base_reset_agent_p2a p2a = {
		.status = SCMI_SUCCESS,
	};

	if (msg->in_size != sizeof(*a2p)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	a2p = (void *)msg->in;

	if (a2p->flags & SCMI_RESET_FLAG_MASK)
		p2a.status = plat_scmi_reset_agent(a2p->agent_id);

	scmi_write_response(msg, &p2a, sizeof(p2a));
}

static const scmi_msg_handler_t scmi_base_handler_table[] = {
	[SCMI_PROTOCOL_VERSION] = report_version,
	[SCMI_PROTOCOL_ATTRIBUTES] = report_attributes,
	[SCMI_PROTOCOL_MESSAGE_ATTRIBUTES] = report_message_attributes,
	[SCMI_BASE_DISCOVER_VENDOR] = discover_vendor,
	[SCMI_BASE_DISCOVER_SUB_VENDOR] = discover_sub_vendor,
	[SCMI_BASE_DISCOVER_IMPLEMENTATION_VERSION] =
					discover_implementation_version,
	[SCMI_BASE_DISCOVER_LIST_PROTOCOLS] = discover_list_protocols,
	[SCMI_BASE_DISCOVER_AGENT] = discover_agent,
	[SCMI_BASE_RESET_AGENT_CONFIG] = reset_agent_config,
	[SCMI_BASE_NOTIFY_ERRORS] = base_notify_errors,
};

static bool message_id_is_supported(unsigned int message_id)
{
	return (message_id < ARRAY_SIZE(scmi_base_handler_table)) &&
	       (scmi_base_handler_table[message_id] != NULL);
}

scmi_msg_handler_t scmi_msg_get_base_handler(struct scmi_msg *msg)
{
	unsigned int message_id = SPECULATION_SAFE_VALUE(msg->message_id);

	if (message_id >= ARRAY_SIZE(scmi_base_handler_table)) {
		VERBOSE("Base handle not found %u\n", msg->message_id);
		return NULL;
	}

	return scmi_base_handler_table[message_id];
}
