/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <cdefs.h>
#include <clk/s32gen1_scmi_clk.h>
#include <drivers/st/scmi-msg.h>
#include <drivers/st/scmi.h>
#include <errno.h>

#define S32GEN1_CLK_DRV_NAME	"clks"

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

	*nb_elts = 1;

	/* One single rate. Frequency scaling to be added */
	if (rates == NULL)
		return SCMI_SUCCESS;

	drv = get_clk_driver_by_name(S32GEN1_CLK_DRV_NAME);
	clk.drv = drv;
	clk.id = scmi_id;

	ret = s32gen1_scmi_clk_get_rates(&clk, rates, nb_elts);
	if (ret == -EINVAL)
		return SCMI_INVALID_PARAMETERS;

	return 0;
}

int32_t plat_scmi_clock_rates_by_step(unsigned int agent_id,
				      unsigned int scmi_id,
				      unsigned long *min_max_step)
{
	return -EINVAL;
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

	drv = get_clk_driver_by_name(S32GEN1_CLK_DRV_NAME);
	clk.drv = drv;
	clk.id = scmi_id;

	if (s32gen1_scmi_clk_set_rate(&clk, rate) != rate)
		return -EINVAL;

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

	drv = get_clk_driver_by_name(S32GEN1_CLK_DRV_NAME);
	clk.drv = drv;
	clk.id = scmi_id;

	return s32gen1_scmi_enable(&clk, enable_not_disable);
}
