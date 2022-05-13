/*
 * Copyright 2019-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch_helpers.h>
#include <assert.h>
#include <common/bl_common.h>
#include <drivers/arm/gicv3.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <libfdt.h>
#include <psci.h>
#include <plat/common/platform.h>

#include <drivers/generic_delay_timer.h>
#include <ocotp.h>
#include <platform_def.h>
#include <pmic/vr5510.h>
#include <s32g_pm.h>
#include <s32g_clocks.h>
#include <s32_dt.h>
#include <s32g_mc_me.h>
#include <s32g_pinctrl.h>
#include <s32gen1-wkpu.h>
#include <s32g_bl_common.h>
#include <clk/clk.h>

static void dt_init_wkpu(void)
{
	void *fdt;
	int wkpu_node;
	int ret;

	if (dt_open_and_check() < 0) {
		INFO("ERROR fdt check\n");
		return;
	}

	if (fdt_get_address(&fdt) == 0) {
		INFO("ERROR fdt\n");
		return;
	}

	wkpu_node = fdt_node_offset_by_compatible(fdt, -1,
			"nxp,s32cc-wkpu");
	if (wkpu_node == -1)
		return;


	ret = s32gen1_wkpu_init(fdt, wkpu_node);
	if (ret) {
		INFO("Failed to initialize WKPU\n");
		return;
	}
}

static int check_clock_node(const void *fdt, int nodeoffset)
{
	const void *prop;
	int len;

	prop = fdt_getprop(fdt, nodeoffset, "assigned-clocks", &len);
	if (!prop)
		return len;

	return 0;

}

static int next_node_with_clocks(const void *fdt, int startoffset)
{
	int offset, err;

	for (offset = fdt_next_node(fdt, startoffset, NULL);
	     offset >= 0;
	     offset = fdt_next_node(fdt, offset, NULL)) {
		err = check_clock_node(fdt, offset);
		if ((err < 0) && (err != -FDT_ERR_NOTFOUND))
			return err;
		else if (err == 0)
			return offset;
	}

	return offset; /* error from fdt_next_node() */
}

void clk_tree_init(void)
{
	void *fdt;
	int clk_node;

	if (dt_open_and_check() < 0) {
		INFO("ERROR fdt check\n");
		return;
	}

	if (fdt_get_address(&fdt) == 0) {
		INFO("ERROR fdt\n");
		return;
	}

	clk_node = -1;
	while (true) {
		clk_node = next_node_with_clocks(fdt, clk_node);
		if (clk_node == -1)
			break;
	}
}

void bl31_platform_setup(void)
{
	generic_delay_timer_init();

	dt_init_pmic();
	dt_init_wkpu();
	dt_init_ocotp();

	update_core_state(plat_my_core_pos(), 1);
	s32_gic_setup();

	s32_enable_a53_clock();
	dt_clk_init();
}

