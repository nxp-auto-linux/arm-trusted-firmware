/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SCMI_MSG_PERF_H
#define SCMI_MSG_PERF_H

#define SCMI_PROTOCOL_VERSION_PERF  0x20000U

/*
 * Identifiers of the SCMI Performance Domain Management Protocol commands
 */
enum scmi_perf_command_id {
	SCMI_PERFORMANCE_DOMAIN_ATTRIBUTES = 0x3,
	SCMI_PERFORMANCE_DESCRIBE_LEVELS = 0x4,
	SCMI_PERFORMANCE_LIMITS_SET = 0x5,
	SCMI_PERFORMANCE_LIMITS_GET = 0x6,
	SCMI_PERFORMANCE_LEVEL_SET = 0x7,
	SCMI_PERFORMANCE_LEVEL_GET = 0x8,
};

/* Protocol attributes */
#define SCMI_PERF_PERF_DOMAIN_COUNT_MASK	GENMASK(15, 0)
#define SCMI_PERF_POWER_MW_MASK				BIT(16)
#define SCMI_PERF_POWER_MW_POS				16

#define SCMI_PERF_PROTOCOL_ATTRIBUTES(_power_mw, _perf_domain_count) \
	((((_power_mw) << SCMI_PERF_POWER_MW_POS) & SCMI_PERF_POWER_MW_MASK) | \
	((_perf_domain_count) & SCMI_PERF_PERF_DOMAIN_COUNT_MASK))

struct scmi_protocol_attributes_p2a_perf {
	int32_t status;
	uint32_t attributes;
	uint32_t statistics_addr_low;
	uint32_t statistics_addr_high;
	uint32_t statistics_len;
};

/* Domain attributes */
struct scmi_performance_domain_attributes_a2p {
	uint32_t domain_id;
};

#define SCMI_PERF_DOMAIN_ATTRIBUTES_CAN_SET_PERF_LEVEL_POS  30
#define SCMI_PERF_DOMAIN_ATTRIBUTES_CAN_SET_PERF_LEVEL_MASK  \
	BIT(SCMI_PERF_DOMAIN_ATTRIBUTES_CAN_SET_PERF_LEVEL_POS)

#define SCMI_DOMAIN_NAME_LENGTH_MAX		16U
struct scmi_performance_domain_attributes_p2a {
	int32_t status;
	uint32_t attributes;
	uint32_t rate_limit;
	uint32_t sustained_freq_khz;
	uint32_t sustained_perf_level;
	char name[SCMI_DOMAIN_NAME_LENGTH_MAX];
};

/*
 * Describe Levels
 */
#define SCMI_PERF_DESCRIBE_LEVELS_REMAINING_MASK	GENMASK_32(31, 16)
#define SCMI_PERF_DESCRIBE_LEVELS_REMAINING_POS		16

#define SCMI_PERF_DESCRIBE_LEVELS_COUNT_MASK		GENMASK_32(11, 0)

#define SCMI_PERF_DESCRIBE_LEVELS_NUM_LEVELS_FLAGS(_count, _rem_rates) \
	( \
		((_count) & SCMI_PERF_DESCRIBE_LEVELS_COUNT_MASK) | \
		(((_rem_rates) << SCMI_PERF_DESCRIBE_LEVELS_REMAINING_POS) & \
		 SCMI_PERF_DESCRIBE_LEVELS_REMAINING_MASK) \
	)

struct scmi_performance_describe_levels_a2p {
	uint32_t domain_id;
	uint32_t level_index;
};

struct scmi_perf_level {
	uint32_t perf_value;
	uint32_t power_cost;
	uint32_t attributes;
};

struct scmi_performance_describe_levels_p2a {
	int32_t status;
	uint32_t num_levels;
	struct scmi_perf_level perf_levels[];
};

/*
 * Limits Set
 */
struct scmi_performance_limits_set_a2p {
	uint32_t domain_id;
	uint32_t range_max;
	uint32_t range_min;
};

/*
 * Limits Get
 */
struct scmi_performance_limits_get_a2p {
	uint32_t domain_id;
};

struct scmi_performance_limits_set_p2a {
	int32_t status;
	uint32_t range_max;
	uint32_t range_min;
};

/*
 * Level Set
 */
struct scmi_performance_level_set_a2p {
	uint32_t domain_id;
	uint32_t performance_level;
};

/*
 * Level Get
 */
struct scmi_performance_level_get_a2p {
	uint32_t domain_id;
};

struct scmi_performance_level_get_p2a {
	int32_t status;
	uint32_t performance_level;
};

#endif /* SCMI_MSG_PERF_H */
