/*
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cdefs.h>
#include <clk/s32gen1_scmi_clk.h>
#include <common/debug.h>
#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>
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
		ERROR("Agent %d is invalid!\n", agent_id);
		return false;
	}

	return true;
}

static bool are_agent_clk_valid(unsigned int agent_id, unsigned int clk_id)
{
	if (!is_agent_valid(agent_id))
		return false;

	if (clk_id >= ARRAY_SIZE(clk_states[0]))
		return false;

	return true;
}

static bool can_set_clk_state(unsigned int agent_id, unsigned int clk_id,
			      bool enable)
{
	if (!are_agent_clk_valid(agent_id, clk_id))
		return false;

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

static bool is_clk_enabled(unsigned int agent_id, unsigned int clk_id)
{
	return clk_states[agent_id][clk_id] > 0;
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

size_t plat_scmi_clock_count(unsigned int agent_id)
{
	if (!is_agent_valid(agent_id))
		return 0;

	return s32gen1_scmi_nclocks();
}

const char *plat_scmi_clock_get_name(unsigned int agent_id,
				     unsigned int scmi_id)
{
	if (!are_agent_clk_valid(agent_id, scmi_id))
		return NULL;

	return s32gen1_scmi_clk_get_name(scmi_id);
}

int32_t plat_scmi_clock_rates_array(unsigned int agent_id, unsigned int scmi_id,
				    unsigned long *rates, size_t *nb_elts)
{
	struct clk_driver *drv;
	struct clk clk;
	int ret;

	if (!are_agent_clk_valid(agent_id, scmi_id))
		return SCMI_INVALID_PARAMETERS;

	*nb_elts = 2;

	if (rates == NULL)
		return SCMI_SUCCESS;

	drv = get_clk_driver_by_name(S32GEN1_CLK_DRV_NAME);
	clk.drv = drv;
	clk.data = NULL;
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

	if (!are_agent_clk_valid(agent_id, scmi_id))
		return 0;

	drv = get_clk_driver_by_name(S32GEN1_CLK_DRV_NAME);
	clk.drv = drv;
	clk.data = NULL;
	clk.id = scmi_id;

	return s32gen1_scmi_clk_get_rate(&clk);
}

int32_t plat_scmi_clock_set_rate(unsigned int agent_id, unsigned int scmi_id,
				 unsigned long rate)
{
	struct clk_driver *drv;
	struct clk clk;
	int ret;

	if (!are_agent_clk_valid(agent_id, scmi_id))
		return SCMI_INVALID_PARAMETERS;

	/* Already running at the requested frequency */
	if (is_clk_enabled(agent_id, scmi_id) &&
	    plat_scmi_clock_get_rate(agent_id, scmi_id) == rate)
		return SCMI_SUCCESS;

	drv = get_clk_driver_by_name(S32GEN1_CLK_DRV_NAME);
	clk.drv = drv;
	clk.data = NULL;
	clk.id = scmi_id;

	/**
	 * Limitation: The rate of a clock cannot be
	 * changed once it's enabled.
	 */
	if (is_clk_enabled(agent_id, scmi_id)) {
		/**
		 * Best effort. It will only succeed if the
		 * rate change is limited to a single divider.
		 */
		ret = s32gen1_scmi_enable(&clk, false);
		if (ret)
			return ret;
	}

	if (s32gen1_scmi_clk_set_rate(&clk, rate) != rate)
		return SCMI_INVALID_PARAMETERS;

	return s32gen1_scmi_enable(&clk, true);
}

int32_t plat_scmi_clock_get_state(unsigned int agent_id, unsigned int scmi_id)
{
	if (!are_agent_clk_valid(agent_id, scmi_id))
		return SCMI_INVALID_PARAMETERS;

	if (is_clk_enabled(agent_id, scmi_id))
		return 1;

	return 0;
}

int32_t plat_scmi_clock_set_state(unsigned int agent_id, unsigned int scmi_id,
				  bool enable_not_disable)
{
	struct clk_driver *drv;
	struct clk clk;
	int ret;

	if (!are_agent_clk_valid(agent_id, scmi_id))
		return SCMI_INVALID_PARAMETERS;

	drv = get_clk_driver_by_name(S32GEN1_CLK_DRV_NAME);
	clk.drv = drv;
	clk.data = NULL;
	clk.id = scmi_id;

	if (!can_set_clk_state(agent_id, scmi_id, enable_not_disable))
		return SCMI_INVALID_PARAMETERS;

	ret = s32gen1_scmi_enable(&clk, enable_not_disable);
	if (!ret)
		update_clk_refcnt(agent_id, scmi_id, enable_not_disable);

	return ret;
}

