/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cdefs.h>
#include <string.h>

#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>
#include <lib/utils.h>
#include <lib/utils_def.h>

#include "common.h"

static bool message_id_is_supported(size_t message_id);

#pragma weak plat_scmi_perf_domain_count
#pragma weak plat_scmi_perf_is_power_mw
#pragma weak plat_scmi_perf_statistics
#pragma weak plat_scmi_perf_get_name
#pragma weak plat_scmi_perf_get_attributes
#pragma weak plat_scmi_perf_get_sustained_freq
#pragma weak plat_scmi_perf_get_sustained_perf_lvl
#pragma weak plat_scmi_perf_describe_levels
#pragma weak plat_scmi_perf_set_limits
#pragma weak plat_scmi_perf_get_limits
#pragma weak plat_scmi_perf_set_level
#pragma weak plat_scmi_perf_get_level

size_t plat_scmi_perf_domain_count(unsigned int agent_id __unused)
{
	return 0U;
}

bool plat_scmi_perf_is_power_mw(unsigned int agent_id __unused)
{
	return false;
}

unsigned int plat_scmi_perf_statistics(unsigned int agent_id __unused,
				    unsigned long *addr __unused)
{
	return 0U;
}

const char *plat_scmi_perf_get_name(unsigned int agent_id __unused,
				  unsigned int domain_id __unused)
{
	return NULL;
}

int32_t plat_scmi_perf_get_attributes(unsigned int agent_id __unused,
					unsigned int domain_id __unused)
{
	return SCMI_NOT_SUPPORTED;
}

unsigned int plat_scmi_perf_get_sustained_freq(unsigned int agent_id __unused,
					unsigned int domain_id __unused)
{
	return 0U;
}

unsigned int plat_scmi_perf_get_sustained_perf_lvl(unsigned int agent_id __unused,
					unsigned int domain_id __unused)
{
	return 0U;
}

int32_t plat_scmi_perf_describe_levels(unsigned int agent_id __unused,
				    unsigned int domain_id __unused, size_t lvl_index __unused,
				    struct scmi_perf_level *levels __unused,
				    size_t *num_levels __unused)
{
	return SCMI_NOT_SUPPORTED;
}

int32_t plat_scmi_perf_set_limits(unsigned int agent_id __unused,
				    unsigned int domain_id __unused,
				    unsigned int range_max __unused,
				    unsigned int range_min __unused)
{
	return SCMI_NOT_SUPPORTED;
}

int32_t plat_scmi_perf_get_limits(unsigned int agent_id __unused,
				    unsigned int domain_id __unused,
				    unsigned int *range_max __unused,
				    unsigned int *range_min __unused)
{
	return SCMI_NOT_SUPPORTED;
}

int32_t plat_scmi_perf_set_level(unsigned int agent_id __unused,
				    unsigned int domain_id __unused,
					unsigned int perf_level __unused)
{
	return SCMI_NOT_SUPPORTED;
}

int32_t plat_scmi_perf_get_level(unsigned int agent_id __unused,
				    unsigned int domain_id __unused,
				    unsigned int *perf_level __unused)
{
	return SCMI_NOT_SUPPORTED;
}


static void report_version(struct scmi_msg *msg)
{
	struct scmi_protocol_version_p2a return_values = {
		.status = SCMI_SUCCESS,
		.version = SCMI_PROTOCOL_VERSION_PERF,
	};

	if (msg->in_size != 0U) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static void report_attributes(struct scmi_msg *msg)
{
	size_t domain_count = plat_scmi_perf_domain_count(msg->agent_id);
	bool power_mw = plat_scmi_perf_is_power_mw(msg->agent_id);
	unsigned long addr = 0UL;
	unsigned int len;

	struct scmi_protocol_attributes_p2a_perf return_values = {
		.status = SCMI_SUCCESS,
		.attributes = SCMI_PERF_PROTOCOL_ATTRIBUTES(power_mw, domain_count),
	};

	if (msg->in_size != 0) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	len = plat_scmi_perf_statistics(msg->agent_id, &addr);
	if (len != 0U) {
		return_values.statistics_addr_low = (unsigned int)addr;
		return_values.statistics_addr_high = (uint32_t)(addr >> 32);
		return_values.statistics_len = len;
	}

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static void report_message_attributes(struct scmi_msg *msg)
{
	struct scmi_protocol_message_attributes_a2p *in_args = (void *)msg->in;
	struct scmi_protocol_message_attributes_p2a return_values = {
		.status = SCMI_SUCCESS,
		.attributes = 0U, /* FastChannels unsupported */
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

static void scmi_performance_domain_attributes(struct scmi_msg *msg)
{
	struct scmi_performance_domain_attributes_a2p *in_args = (void *)msg->in;
	struct scmi_performance_domain_attributes_p2a return_values = {
		.status = SCMI_SUCCESS,
	};
	const char *name = NULL;
	unsigned int domain_id = 0U;

	if (msg->in_size != sizeof(*in_args)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	domain_id = SPECULATION_SAFE_VALUE(in_args->domain_id);
	if (domain_id > plat_scmi_perf_domain_count(msg->agent_id)) {
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	name = plat_scmi_perf_get_name(msg->agent_id, domain_id);
	if (name == NULL) {
		scmi_status_response(msg, SCMI_NOT_FOUND);
		return;
	}

	COPY_NAME_IDENTIFIER(return_values.name, name);

	return_values.attributes = plat_scmi_perf_get_attributes(msg->agent_id, domain_id);
	return_values.rate_limit = 0U;
	return_values.sustained_freq_khz =
		plat_scmi_perf_get_sustained_freq(msg->agent_id, domain_id);
	return_values.sustained_perf_level =
		plat_scmi_perf_get_sustained_perf_lvl(msg->agent_id, domain_id);

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

#define LEVELS_SIZE_MAX		(SCMI_PLAYLOAD_MAX - \
				 sizeof(struct scmi_performance_describe_levels_p2a))

#define PERF_LEVEL_SIZE		sizeof(struct scmi_perf_level)

#define MAX_LEVELS			((LEVELS_SIZE_MAX) / (PERF_LEVEL_SIZE))

static void scmi_performance_describe_levels(struct scmi_msg *msg)
{
	struct scmi_performance_describe_levels_a2p *in_args = (void *)msg->in;
	struct scmi_performance_describe_levels_p2a p2a = {
		.status = SCMI_SUCCESS,
	};
	unsigned int domain_id = 0U;
	int32_t status;
	size_t num_levels;
	struct scmi_perf_level plat_levels[MAX_LEVELS];

	if (msg->in_size != sizeof(*in_args)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	domain_id = SPECULATION_SAFE_VALUE(in_args->domain_id);
	if (domain_id > plat_scmi_perf_domain_count(msg->agent_id)) {
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	num_levels = MAX_LEVELS;
	status = plat_scmi_perf_describe_levels(msg->agent_id, domain_id,
					in_args->level_index, plat_levels, &num_levels);
	if (status == SCMI_SUCCESS) {
		size_t ret_nb = MIN(num_levels - in_args->level_index, MAX_LEVELS);
		size_t rem_nb = num_levels - in_args->level_index - ret_nb;

		p2a.num_levels = SCMI_PERF_DESCRIBE_LEVELS_NUM_LEVELS_FLAGS(ret_nb, rem_nb);
		memcpy(msg->out, &p2a, sizeof(p2a));
		memcpy(msg->out + sizeof(p2a), plat_levels, ret_nb * PERF_LEVEL_SIZE);
		msg->out_size_out = sizeof(p2a) + ret_nb * PERF_LEVEL_SIZE;
	} else
		scmi_status_response(msg, status);
}

static void scmi_performance_limits_set(struct scmi_msg *msg)
{
	const struct scmi_performance_limits_set_a2p *in_args = (void *)msg->in;
	int32_t status;
	unsigned int domain_id = 0U;
	unsigned int range_min = 0U, range_max = 0U;

	if (msg->in_size != sizeof(*in_args)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	domain_id = SPECULATION_SAFE_VALUE(in_args->domain_id);
	if (domain_id > plat_scmi_perf_domain_count(msg->agent_id)) {
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	range_max = SPECULATION_SAFE_VALUE(in_args->range_max);
	range_min = SPECULATION_SAFE_VALUE(in_args->range_min);

	status = plat_scmi_perf_set_limits(msg->agent_id, domain_id, range_max, range_min);

	scmi_status_response(msg, status);
}

static void scmi_performance_limits_get(struct scmi_msg *msg)
{
	const struct scmi_performance_limits_get_a2p *in_args = (void *)msg->in;
	struct scmi_performance_limits_set_p2a return_values = {
		.status = SCMI_SUCCESS,
	};
	int32_t status;
	unsigned int domain_id = 0U;
	unsigned int range_max = 0U, range_min = 0U;

	if (msg->in_size != sizeof(*in_args)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	domain_id = SPECULATION_SAFE_VALUE(in_args->domain_id);
	if (domain_id > plat_scmi_perf_domain_count(msg->agent_id)) {
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	status = plat_scmi_perf_get_limits(msg->agent_id, domain_id, &range_max, &range_min);
	if (status != SCMI_SUCCESS) {
		scmi_status_response(msg, status);
		return;
	}

	return_values.range_max = range_max;
	return_values.range_min = range_min;

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static void scmi_performance_level_set(struct scmi_msg *msg)
{
	const struct scmi_performance_level_set_a2p *in_args = (void *)msg->in;
	int32_t status;
	unsigned int domain_id = 0U;
	unsigned int performance_level = 0U;

	if (msg->in_size != sizeof(*in_args)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	domain_id = SPECULATION_SAFE_VALUE(in_args->domain_id);
	if (domain_id > plat_scmi_perf_domain_count(msg->agent_id)) {
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	performance_level = SPECULATION_SAFE_VALUE(in_args->performance_level);

	status = plat_scmi_perf_set_level(msg->agent_id, domain_id, performance_level);

	scmi_status_response(msg, status);
}

static void scmi_performance_level_get(struct scmi_msg *msg)
{
	const struct scmi_performance_level_get_a2p *in_args = (void *)msg->in;
	struct scmi_performance_level_get_p2a return_values = {
		.status = SCMI_SUCCESS,
	};
	int32_t status;
	unsigned int domain_id = 0U;
	unsigned int performance_level = 0U;

	if (msg->in_size != sizeof(*in_args)) {
		scmi_status_response(msg, SCMI_PROTOCOL_ERROR);
		return;
	}

	domain_id = SPECULATION_SAFE_VALUE(in_args->domain_id);
	if (domain_id > plat_scmi_perf_domain_count(msg->agent_id)) {
		scmi_status_response(msg, SCMI_INVALID_PARAMETERS);
		return;
	}

	status = plat_scmi_perf_get_level(msg->agent_id, domain_id, &performance_level);
	if (status != SCMI_SUCCESS) {
		scmi_status_response(msg, status);
		return;
	}

	return_values.performance_level = performance_level;

	scmi_write_response(msg, &return_values, sizeof(return_values));
}

static const scmi_msg_handler_t scmi_perf_handler_table[] = {
	[SCMI_PROTOCOL_VERSION] = report_version,
	[SCMI_PROTOCOL_ATTRIBUTES] = report_attributes,
	[SCMI_PROTOCOL_MESSAGE_ATTRIBUTES] = report_message_attributes,
	[SCMI_PERFORMANCE_DOMAIN_ATTRIBUTES] = scmi_performance_domain_attributes,
	[SCMI_PERFORMANCE_DESCRIBE_LEVELS] = scmi_performance_describe_levels,
	[SCMI_PERFORMANCE_LIMITS_SET] = scmi_performance_limits_set,
	[SCMI_PERFORMANCE_LIMITS_GET] = scmi_performance_limits_get,
	[SCMI_PERFORMANCE_LEVEL_SET] = scmi_performance_level_set,
	[SCMI_PERFORMANCE_LEVEL_GET] = scmi_performance_level_get,

};

static bool message_id_is_supported(size_t message_id)
{
	return (message_id < ARRAY_SIZE(scmi_perf_handler_table)) &&
	       (scmi_perf_handler_table[message_id] != NULL);
}

scmi_msg_handler_t scmi_msg_get_perf_handler(struct scmi_msg *msg)
{
	unsigned int message_id = SPECULATION_SAFE_VALUE(msg->message_id);

	if (message_id >= ARRAY_SIZE(scmi_perf_handler_table)) {
		VERBOSE("Performance management domain handle not found %u\n", msg->message_id);
		return NULL;
	}

	return scmi_perf_handler_table[message_id];
}

