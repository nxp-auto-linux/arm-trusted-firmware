// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022-2024 NXP
 */

#include <clk/clk.h>
#include <clk/s32gen1_scmi_clk.h>
#include <clk/s32gen1_scmi_perf.h>
#include <drivers/scmi.h>
#include <drivers/scmi-msg.h>
#include <dt-bindings/perf/s32cc-scmi-perf.h>
#include <lib/utils_def.h>
#include <lib/spinlock.h>
#include <s32cc_svc.h>

struct opp {
	uint32_t level;
	unsigned long frequency;
};

/**
 * Mapping between performance level and frequency.
 */
static struct opp opps[S32CC_SCMI_PERF_MAX_ID][S32GEN1_SCMI_MAX_LEVELS];
static spinlock_t opps_lock;

static int32_t populate_opps_table(unsigned int domain_id, size_t lvl_index,
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

static uint32_t find_perf_level_by_rate(unsigned int domain_id, unsigned long rate)
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

static unsigned long find_rate_by_perf_level(unsigned int domain_id, uint32_t perf_level)
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

/**
 * Copy the available clock rates returned by calling `plat_scmi_clock_rates_array`
 * into the buffer describing possible performance levels for a given clock.
 */
int32_t s32gen1_scmi_get_perf_levels(unsigned int agent_id, unsigned int clock_id,
	unsigned int domain_id, size_t lvl_index, uint32_t *levels, size_t *num_levels)
{
	unsigned long rates[S32GEN1_MAX_NUM_FREQ] = {0};
	size_t num_rates = 0, i;
	int32_t ret = SCMI_SUCCESS;

	ret = plat_scmi_clock_rates_array(agent_id, clock_id, rates,
					  &num_rates, 0);
	if (ret != SCMI_SUCCESS)
		return ret;

	ret = populate_opps_table(domain_id, lvl_index, rates, num_rates);
	if (ret != SCMI_SUCCESS)
		return ret;

	/* copy requested performance levels to buffer */
	for (i = 0; i < *num_levels && i < num_rates; i++) {

		if (i + lvl_index >= ARRAY_SIZE(opps[domain_id]))
			break;

		spin_lock(&opps_lock);
		levels[3 * i] = opps[domain_id][i + lvl_index].level;
		spin_unlock(&opps_lock);
		levels[3 * i + 1] = 0; /* power cost */
		levels[3 * i + 2] = 0; /* attributes */
	}

	/* return the number of all available perf levels */
	*num_levels = num_rates;

	return ret;
}

unsigned int s32gen1_scmi_get_level(unsigned int agent_id, unsigned int clock_id,
				unsigned int domain_id)
{
	unsigned long rate = plat_scmi_clock_get_rate(agent_id, clock_id);

	return find_perf_level_by_rate(domain_id, rate);
}

int s32gen1_scmi_set_level(unsigned int agent_id, unsigned int clock_id, unsigned int domain_id,
	unsigned int perf_level)
{
	unsigned long rate = find_rate_by_perf_level(domain_id, perf_level);

	if (!is_plat_agent(agent_id))
		return SCMI_DENIED;

	return plat_scmi_clock_set_rate(agent_id, clock_id, rate);
}

unsigned int s32gen1_scmi_get_max_level(unsigned int domain_id, uint32_t clock_id)
{
	struct clk_driver *drv = get_clk_driver_by_name(S32GEN1_CLK_DRV_NAME);
	struct clk clk;
	unsigned long rate;

	clk.drv = drv;
	clk.id = clock_id;
	rate = s32gen1_get_maxrate(&clk);

	return find_perf_level_by_rate(domain_id, rate);
}

unsigned int s32gen1_scmi_get_min_level(unsigned int domain_id, uint32_t clock_id)
{
	struct clk_driver *drv = get_clk_driver_by_name(S32GEN1_CLK_DRV_NAME);
	struct clk clk;
	unsigned long rate;

	clk.drv = drv;
	clk.id = clock_id;
	rate = s32gen1_get_minrate(&clk);

	return find_perf_level_by_rate(domain_id, rate);
}

