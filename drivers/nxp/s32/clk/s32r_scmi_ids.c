// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */
#include <dt-bindings/clock/s32r45-clock.h>
#include <dt-bindings/clock/s32r45-scmi-clock.h>
#include <clk/s32gen1_clk_funcs.h>
#include <clk/s32gen1_scmi_clk.h>
#include <errno.h>
#include <stdint.h>

#define INDEX(X)	((X) - S32GEN1_SCMI_PLAT_CLK_BASE_ID)

struct s32gen1_scmi_clk s32r45_scmi_clk[] = {
    /* LAX */
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_LAX_MODULE,
		S32R45_CLK_ACCEL4, "lax_module"),
    /* SPT */
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_SPT_SPT,
		S32R45_CLK_ACCEL3, "spt_spt"),
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_SPT_AXI,
		S32R45_CLK_ACCEL3, "spt_axi"),
	SCMI_ARRAY_ENTRY(S32R45_SCMI_CLK_SPT_MODULE,
		S32R45_CLK_ACCEL3_DIV3, "spt_module"),
};

int plat_scmi_id2clk(uint32_t scmi_clk_id, uint32_t *clk_id)
{
	if (!clk_id)
		return -EINVAL;

	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(s32r45_scmi_clk))
		return -EINVAL;

	*clk_id = s32r45_scmi_clk[INDEX(scmi_clk_id)].plat_id;
	if (!*clk_id) {
		ERROR("Unhandled S32R clock: %u\n", scmi_clk_id);
		return -EINVAL;
	}

	return 0;
}

int plat_compound_clk_get(struct clk *clk)
{
	return -EINVAL;
}

int plat_compound_clk_set_parents(struct clk *clk)
{
	return -EINVAL;
}

int plat_compound_clk_enable(struct clk *clk, int enable)
{
	return -EINVAL;
}

unsigned long plat_compound_clk_set_rate(struct clk *clk, unsigned long rate)
{
	return 0;
}

unsigned long plat_compound_clk_get_rate(struct clk *clk)
{
	return 0;
}

uint32_t plat_get_nclocks(void)
{
	return S32GEN1_PLAT_SCMI_CLK(ARRAY_SIZE(s32r45_scmi_clk));
}

const char *plat_scmi_clk_get_name(uint32_t scmi_clk_id)
{
	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(s32r45_scmi_clk))
		return NULL;

	return s32r45_scmi_clk[INDEX(scmi_clk_id)].name;
}

bool plat_scmi_clk_is_enabled(uint32_t scmi_clk_id)
{
	if (INDEX(scmi_clk_id) >= ARRAY_SIZE(s32r45_scmi_clk))
		return false;

	return s32r45_scmi_clk[INDEX(scmi_clk_id)].enabled;
}

int plat_scmi_clk_get_rates(struct clk *clk, unsigned long *rates,
			    size_t *nrates)
{
	return 0;
}

