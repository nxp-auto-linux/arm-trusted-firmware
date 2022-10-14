// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */
#include <clk/s32gen1_scmi_perf.h>
#include <drivers/scmi.h>
#include <dt-bindings/perf/s32gen1-scmi-perf.h>
#include <lib/utils_def.h>
#include <lib/spinlock.h>

struct opp {
	uint32_t level;
	unsigned long frequency;
};

/**
 * Mapping between performance level and frequency.
 */
static struct opp opps[S32GEN1_SCMI_PERF_MAX_ID][S32GEN1_SCMI_MAX_LEVELS];
static spinlock_t opps_lock;

int32_t populate_opps_table(unsigned int domain_id, size_t lvl_index,
		unsigned long *rates, size_t num_rates)
{
	size_t i = 0;
	unsigned long level;

	if (domain_id >= ARRAY_SIZE(opps))
		return SCMI_INVALID_PARAMETERS;

	for (i = lvl_index; i < num_rates; i++) {

		if (i >= ARRAY_SIZE(opps[domain_id]))
			break;

		level = rate2level(rates[i]);
		if (level > UINT32_MAX)
			return SCMI_INVALID_PARAMETERS;

		spin_lock(&opps_lock);
		opps[domain_id][i].frequency = rates[i];
		opps[domain_id][i].level = level;
		spin_unlock(&opps_lock);

	}

	return SCMI_SUCCESS;
}

uint32_t find_perf_level_by_rate(unsigned int domain_id, unsigned long rate)
{
	size_t i = 0;
	uint32_t level = 0;

	if (domain_id >= ARRAY_SIZE(opps))
		return 0;

	spin_lock(&opps_lock);
	for (i = 0; i < ARRAY_SIZE(opps[domain_id]); i++) {
		if (opps[domain_id][i].frequency == rate) {
			level = opps[domain_id][i].level;
			break;
		}
	}
	spin_unlock(&opps_lock);

	return level;
}

unsigned long find_rate_by_perf_level(unsigned int domain_id, uint32_t perf_level)
{
	size_t i = 0;
	unsigned long rate = 0;

	if (domain_id >= ARRAY_SIZE(opps))
		return 0;

	spin_lock(&opps_lock);
	for (i = 0; i < ARRAY_SIZE(opps[domain_id]); i++) {
		if (opps[domain_id][i].level == perf_level) {
			rate = opps[domain_id][i].frequency;
			break;
		}
	}
	spin_unlock(&opps_lock);

	return rate;
}

