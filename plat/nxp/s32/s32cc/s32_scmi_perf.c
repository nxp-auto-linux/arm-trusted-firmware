/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cdefs.h>
#include <clk/s32gen1_scmi_perf.h>
#include <common/debug.h>
#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>
#include <lib/utils_def.h>
#include <s32cc_svc.h>

#include <dt-bindings/clock/s32cc-scmi-clock.h>
#include <dt-bindings/perf/s32cc-scmi-perf.h>

struct perf_domain {
	uint32_t clock_id;
	uint32_t attributes;
	const char *name;
	uint32_t min_level;
	uint32_t max_level;
};

#define PERF_DOMAIN(ID, NAME, ATTR, MIN_LEVEL, MAX_LEVEL) \
{ .clock_id = (ID), .name = (NAME), .attributes = (ATTR), \
	.min_level = (MIN_LEVEL), .max_level = (MAX_LEVEL), }

static struct perf_domain domains[] = {
	[S32CC_SCMI_PERF_A53] = PERF_DOMAIN(S32CC_SCMI_CLK_A53, "a53",
		SCMI_PERF_SET_LIMITS | SCMI_PERF_SET_LEVEL,
		S32GEN1_A53_MIN_LEVEL, S32GEN1_A53_MAX_LEVEL),
};

size_t plat_scmi_perf_domain_count(unsigned int agent_id __unused)
{
	return ARRAY_SIZE(domains);
}

const char *plat_scmi_perf_get_name(unsigned int agent_id __unused,
				  unsigned int domain_id)
{
	if (domain_id >= ARRAY_SIZE(domains))
		return NULL;

	return domains[domain_id].name;
}

int32_t plat_scmi_perf_get_attributes(unsigned int agent_id __unused,
					unsigned int domain_id)
{
	if (domain_id >= ARRAY_SIZE(domains))
		return SCMI_NOT_FOUND;

	return domains[domain_id].attributes;
}

unsigned int plat_scmi_perf_get_sustained_freq(unsigned int agent_id,
					unsigned int domain_id)
{
	unsigned long rate;

	if (domain_id >= ARRAY_SIZE(domains))
		return 0;

	rate = plat_scmi_clock_get_rate(agent_id, domains[domain_id].clock_id);
	rate = rate2khz(rate);
	if (rate > UINT32_MAX)
		return 0;

	return rate;
}

unsigned int plat_scmi_perf_get_sustained_perf_lvl(unsigned int agent_id,
					unsigned int domain_id)
{
	unsigned long rate;

	if (domain_id >= ARRAY_SIZE(domains))
		return 0;

	rate = plat_scmi_clock_get_rate(agent_id, domains[domain_id].clock_id);
	if (rate > UINT32_MAX)
		return 0;

	return rate2level(rate);
}

int32_t plat_scmi_perf_describe_levels(unsigned int agent_id,
				    unsigned int domain_id, size_t lvl_index,
				    struct scmi_perf_level *levels,
				    size_t *num_levels)
{
	unsigned int clock_id;
	uint32_t *buf = (uint32_t *)(uintptr_t)levels;

	if (domain_id >= ARRAY_SIZE(domains))
		return SCMI_NOT_FOUND;

	if (levels == NULL)
		return SCMI_INVALID_PARAMETERS;

	clock_id = domains[domain_id].clock_id;

	return s32gen1_scmi_get_perf_levels(agent_id, clock_id, domain_id,
					lvl_index, buf, num_levels);
}

int32_t plat_scmi_perf_get_level(unsigned int agent_id __unused,
				    unsigned int domain_id __unused,
				    unsigned int *perf_level __unused)
{
	unsigned int clock_id;

	if (domain_id >= ARRAY_SIZE(domains))
		return SCMI_NOT_FOUND;

	if (!perf_level)
		return SCMI_INVALID_PARAMETERS;

	clock_id = domains[domain_id].clock_id;
	*perf_level = s32gen1_scmi_get_level(agent_id, clock_id, domain_id);

	return SCMI_SUCCESS;
}

int32_t plat_scmi_perf_set_level(unsigned int agent_id __unused,
				    unsigned int domain_id __unused,
				    unsigned int perf_level __unused)
{
	unsigned int clock_id;

	if (domain_id >= ARRAY_SIZE(domains))
		return SCMI_NOT_FOUND;

	if (!(domains[domain_id].attributes & SCMI_PERF_SET_LEVEL))
		return SCMI_DENIED;

	if (perf_level > domains[domain_id].max_level ||
		perf_level < domains[domain_id].min_level)
		return SCMI_OUT_OF_RANGE;

	clock_id = domains[domain_id].clock_id;

	/* Only the platform is allowed to set the perf level */
	return s32gen1_scmi_set_level(S32_SCMI_AGENT_PLAT, clock_id, domain_id, perf_level);
}

int32_t plat_scmi_perf_get_limits(unsigned int agent_id, unsigned int domain_id,
				    unsigned int *range_max, unsigned int *range_min)
{
	if (domain_id >= ARRAY_SIZE(domains))
		return SCMI_NOT_FOUND;

	if (!range_max || !range_min)
		return SCMI_INVALID_PARAMETERS;

	*range_max = domains[domain_id].max_level;
	*range_min = domains[domain_id].min_level;

	return SCMI_SUCCESS;
}

int32_t plat_scmi_perf_set_limits(unsigned int agent_id, unsigned int domain_id,
				    unsigned int range_max, unsigned int range_min)
{
	unsigned int max_level, min_level;

	if (domain_id >= ARRAY_SIZE(domains))
		return SCMI_NOT_FOUND;

	if (!(domains[domain_id].attributes & SCMI_PERF_SET_LIMITS))
		return SCMI_DENIED;

	if (range_min > range_max)
		return SCMI_INVALID_PARAMETERS;

	if (range_min == domains[domain_id].min_level &&
		range_max == domains[domain_id].max_level)
		return SCMI_SUCCESS;

	max_level = s32gen1_scmi_get_max_level(domain_id, domains[domain_id].clock_id);
	min_level = s32gen1_scmi_get_min_level(domain_id, domains[domain_id].clock_id);

	if (range_max > max_level ||
			range_min < min_level)
		return SCMI_OUT_OF_RANGE;

	domains[domain_id].max_level = range_max;
	domains[domain_id].min_level = range_min;

	return SCMI_SUCCESS;
}

