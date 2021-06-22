/*
 * Copyright 2020-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cdefs.h>
#include <clk/s32gen1_scmi_clk.h>
#include <common/debug.h>
#include <drivers/st/scmi-msg.h>
#include <drivers/st/scmi.h>
#include <dt-bindings/clock/s32gen1-scmi-clock.h>
#include <errno.h>
#include <lib/utils_def.h>

#ifndef S32GEN1_CLK_MAX_AGENTS
#define S32GEN1_CLK_MAX_AGENTS	2
#endif

/* Clocks state for each agent */
static uint8_t clk_states[S32GEN1_CLK_MAX_AGENTS][S32GEN1_SCMI_CLK_MAX_ID];

static bool is_agent_valid(unsigned int agent_id)
{
	if (agent_id >= ARRAY_SIZE(clk_states)) {
		ERROR("Unable to register agent %d due to %s size\n",
		      agent_id, __STRING(S32GEN1_CLK_MAX_AGENTS));
		return false;
	}

	return true;
}

static bool valid_agent_clk(unsigned int agent_id, unsigned int clk_id,
			    bool enable)
{
	if (!is_agent_valid(agent_id))
		return false;

	if (clk_id >= ARRAY_SIZE(clk_states[0])) {
		ERROR("Unable to register clock %d due to %s size\n",
		      clk_id, __STRING(S32GEN1_SCMI_CLK_MAX_ID));
		return false;
	}

	if (!enable && !clk_states[agent_id][clk_id]) {
		ERROR("Trying to disable a disabled clock %s\n",
		      s32gen1_scmi_clk_get_name(clk_id));
		return false;
	}

	if (enable && (clk_states[agent_id][clk_id] == UINT8_MAX)) {
		ERROR("Too many clock enable operations for clock %s\n",
		      s32gen1_scmi_clk_get_name(clk_id));
		return false;
	}

	return true;
}

int32_t plat_scmi_clock_agent_reset(unsigned int agent_id)
{
	size_t i, j;

	if (!is_agent_valid(agent_id))
		return SCMI_INVALID_PARAMETERS;

	for (i = 0; i < ARRAY_SIZE(clk_states[agent_id]); i++) {
		if (!clk_states[agent_id][i])
			continue;

		for (j = 0; j < clk_states[agent_id][i]; j++)
			plat_scmi_clock_set_state(agent_id, i, false);
	}

	return 0;
}

static void update_clk_refcnt(unsigned int agent_id, unsigned int clk_id,
			      bool enable)
{
	if (!enable)
		clk_states[agent_id][clk_id]--;
	else
		clk_states[agent_id][clk_id]++;
}

size_t plat_scmi_clock_count(unsigned int agent_id __unused)
{
	return s32gen1_scmi_nclocks();
}

const char *plat_scmi_clock_get_name(unsigned int agent_id,
				     unsigned int scmi_id)
{
	return s32gen1_scmi_clk_get_name(scmi_id);
}

int32_t plat_scmi_clock_rates_array(unsigned int agent_id, unsigned int scmi_id,
				    unsigned long *rates, size_t *nb_elts)
{
	struct clk_driver *drv;
	struct clk clk;
	int ret;

	*nb_elts = 2;

	/* One single rate. Frequency scaling to be added */
	if (rates == NULL)
		return SCMI_SUCCESS;

	drv = get_clk_driver_by_name(S32GEN1_CLK_DRV_NAME);
	clk.drv = drv;
	clk.id = scmi_id;

	ret = s32gen1_scmi_clk_get_rates(&clk, rates, nb_elts);
	if (ret == -EINVAL)
		return SCMI_INVALID_PARAMETERS;

	return SCMI_SUCCESS;
}

int32_t plat_scmi_clock_rates_by_step(unsigned int agent_id,
				      unsigned int scmi_id,
				      unsigned long *min_max_step)
{
	return SCMI_NOT_SUPPORTED;
}

unsigned long plat_scmi_clock_get_rate(unsigned int agent_id,
				       unsigned int scmi_id)
{
	struct clk_driver *drv;
	struct clk clk;

	drv = get_clk_driver_by_name(S32GEN1_CLK_DRV_NAME);
	clk.drv = drv;
	clk.id = scmi_id;

	return s32gen1_scmi_clk_get_rate(&clk);
}

int32_t plat_scmi_clock_set_rate(unsigned int agent_id, unsigned int scmi_id,
				 unsigned long rate)
{
	struct clk_driver *drv;
	struct clk clk;

	/* Already running at the requested frequency */
	if (s32gen1_scmi_clk_is_enabled(scmi_id) &&
	    plat_scmi_clock_get_rate(agent_id, scmi_id) == rate)
		return SCMI_SUCCESS;

	/**
	 * Limitation: The rate of a clock cannot be
	 * changed once it's enabled
	 */
	if (s32gen1_scmi_clk_is_enabled(scmi_id))
		return SCMI_DENIED;

	drv = get_clk_driver_by_name(S32GEN1_CLK_DRV_NAME);
	clk.drv = drv;
	clk.id = scmi_id;

	if (s32gen1_scmi_clk_set_rate(&clk, rate) != rate)
		return SCMI_INVALID_PARAMETERS;

	return s32gen1_scmi_enable(&clk, true);
}

int32_t plat_scmi_clock_get_state(unsigned int agent_id, unsigned int scmi_id)
{
	if (s32gen1_scmi_clk_is_enabled(scmi_id))
		return 1;

	return 0;
}

int32_t plat_scmi_clock_set_state(unsigned int agent_id, unsigned int scmi_id,
				  bool enable_not_disable)
{
	struct clk_driver *drv;
	struct clk clk;
	int ret;

	drv = get_clk_driver_by_name(S32GEN1_CLK_DRV_NAME);
	clk.drv = drv;
	clk.id = scmi_id;

	if (!valid_agent_clk(agent_id, scmi_id, enable_not_disable))
		return SCMI_INVALID_PARAMETERS;

	ret = s32gen1_scmi_enable(&clk, enable_not_disable);
	if (!ret)
		update_clk_refcnt(agent_id, scmi_id, enable_not_disable);

	return ret;
}

