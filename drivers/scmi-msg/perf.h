/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SCMI_MSG_PERF_H
#define SCMI_MSG_PERF_H

#define SCMI_PROTOCOL_VERSION_PERF  0x20000U

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

#endif /* SCMI_MSG_PERF_H */
