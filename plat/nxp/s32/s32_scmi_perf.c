/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cdefs.h>
#include <clk/s32gen1_scmi_perf.h>
#include <common/debug.h>
#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>
#include <lib/utils_def.h>

#include <dt-bindings/clock/s32gen1-scmi-clock.h>
#include <dt-bindings/perf/s32gen1-scmi-perf.h>

struct perf_domain {
	uint32_t clock_id;
	uint32_t attributes;
	const char *name;
};

#define PERF_DOMAIN(ID, NAME, ATTR) \
{ .clock_id = (ID), .name = (NAME), .attributes = (ATTR)}

static const struct perf_domain domains[] = {
	[S32GEN1_SCMI_PERF_A53] = PERF_DOMAIN(S32GEN1_SCMI_CLK_A53, "a53",
		SCMI_PERF_SET_LIMITS | SCMI_PERF_SET_LEVEL),
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

