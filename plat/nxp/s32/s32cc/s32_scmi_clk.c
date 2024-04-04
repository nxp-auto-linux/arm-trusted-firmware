/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cdefs.h>
#include <clk/s32gen1_scmi_clk.h>
#include <common/debug.h>
#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>
#include <dt-bindings/clock/s32cc-scmi-clock.h>
#include <errno.h>
#include <lib/utils_def.h>
#include <s32cc_svc.h>

#ifndef S32GEN1_CLK_MAX_AGENTS
#define S32GEN1_CLK_MAX_AGENTS	2
#endif

/* Clocks state for each agent */
static uint8_t clk_states[S32GEN1_CLK_MAX_AGENTS][S32CC_SCMI_CLK_MAX_ID];

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

static uint8_t get_clock_refcount(unsigned int agent_id, unsigned int clk_id)
{
	return clk_states[agent_id][clk_id];
}

int32_t plat_scmi_clock_agent_reset(unsigned int agent_id)
{
	size_t clk_id;
	bool header_printed = false;
	uint8_t refcount, i;

	if (!is_agent_valid(agent_id))
		return SCMI_INVALID_PARAMETERS;

	for (clk_id = 0; clk_id < ARRAY_SIZE(clk_states[agent_id]); clk_id++) {
		refcount = get_clock_refcount(agent_id, clk_id);
		for (i = 0u; i < refcount; i++) {
			if (!i) {
				if (!header_printed) {
					NOTICE("The list of clocks found enabled during the SCMI agent reset command:\n");
					header_printed = true;
				}

				NOTICE("\t%s\n",
				       s32gen1_scmi_clk_get_name(clk_id));
			}

			plat_scmi_clock_set_state(agent_id, clk_id, false);
		}

		if (is_clk_enabled(agent_id, clk_id))
			ERROR("Failed to disable clock: %s\n",
			      s32gen1_scmi_clk_get_name(clk_id));
	}

	return 0;
}

int32_t plat_scmi_clocks_reset_agents(void)
{
	size_t i;
	int32_t ret;

	for (i = 0; i < ARRAY_SIZE(clk_states); i++) {
		/**
		 * Keep the platform clocks in sync with
		 * the hardware state after resume.
		 **/
		if (is_plat_agent(i))
			continue;

		ret = plat_scmi_clock_agent_reset(i);
		if (ret) {
			ERROR("Failed to reset SCMI agent %zu\n", i);
			return ret;
		}
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

void update_a53_clk_state(bool enabled)
{
	update_clk_refcnt(S32_SCMI_AGENT_PLAT, S32CC_SCMI_CLK_A53, enabled);
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
				    unsigned long *rates, size_t *nb_elts,
				    uint32_t start_idx)
{
	struct clk_driver *drv;
	struct clk clk;
	int ret;

	if (!are_agent_clk_valid(agent_id, scmi_id))
		return SCMI_INVALID_PARAMETERS;

	*nb_elts = 0;

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
	bool was_enabled = false;
	unsigned long curr_rate;
	struct clk clk;
	int ret = 0, err;

	if (!are_agent_clk_valid(agent_id, scmi_id))
		return SCMI_INVALID_PARAMETERS;

	curr_rate = plat_scmi_clock_get_rate(agent_id, scmi_id);

	/* Already running at the requested frequency */
	if (is_clk_enabled(agent_id, scmi_id) && curr_rate == rate)
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
		update_clk_refcnt(agent_id, scmi_id, false);
		was_enabled = true;
	}

	if (s32gen1_scmi_clk_set_rate(&clk, rate) != rate) {
		ret = SCMI_INVALID_PARAMETERS;

		/* Best effort: restore previous frequency */
		if (s32gen1_scmi_clk_set_rate(&clk, curr_rate) != curr_rate)
			ERROR("Failed to restore previous rate (%lu) for clock %u\n",
			      curr_rate, scmi_id);
	}

	if (was_enabled) {
		err = s32gen1_scmi_enable(&clk, true);
		if (!err)
			update_clk_refcnt(agent_id, scmi_id, true);
		if (err && !ret)
			ret = err;
	}

	return ret;
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

