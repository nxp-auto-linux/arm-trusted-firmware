/*
 * Copyright 2019-2024 NXP
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
#include <s32cc_dt.h>
#include <s32g_mc_me.h>
#include <s32g_pinctrl.h>
#include <s32gen1-wkpu.h>
#include <s32g_bl_common.h>
#include <clk/clk.h>
#include <s32cc_lowlevel.h>
#include <s32cc_scp_scmi.h>

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
	uintptr_t core_addr;

	generic_delay_timer_init();

	update_core_state(plat_my_core_pos(), CPU_ON, CPU_ON);
	s32_gic_setup();

	if (is_scp_used()) {
		core_addr = (uintptr_t)plat_secondary_cold_boot_setup;
		scp_set_core_reset_addr(core_addr);
	} else {
		dt_init_pmic();
		dt_init_wkpu();
		dt_init_ocotp();

		/* Call the new s32_a53_clock_setup() (which includes xbar_2x
		 * enabling) on the BL31 cold boot path to set early A53 clock
		 * frequency also during BL31. Otherwise, the clock driver will
		 * enforce the frequencies from DT.
		 */
		s32_a53_clock_setup();
		dt_clk_init();

		s32g_reinit_i2c();
	}
}
