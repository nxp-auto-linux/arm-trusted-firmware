/*
 * Copyright 2020-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <lib/mmio.h>
#include <libfdt.h>
#include <drivers/nxp/s32/ocotp.h>
#include <lib/utils_def.h>
#include "platform_def.h"
#include "s32g_pinctrl.h"
#include "s32g_clocks.h"
#include "s32g_bl_common.h"
#include "s32cc_dt.h"
#include "s32g_pinctrl.h"
#include "s32cc-wkpu.h"

extern struct s32_i2c_driver i2c_drivers[];
extern size_t i2c_fill_level;

void s32g_reinit_i2c(void)
{
	size_t i;

	i2c_config_pinctrl();

	for (i = 0; i < i2c_fill_level; i++)
		s32_i2c_init(&i2c_drivers[i].bus);
}

void dt_init_ocotp(void)
{
	void *fdt = NULL;
	int ocotp_node;
	int ret;

	if (dt_open_and_check() < 0) {
		INFO("ERROR fdt check\n");
		return;
	}

	if (fdt_get_address(&fdt) == 0) {
		INFO("ERROR fdt\n");
		return;
	}

	ocotp_node = fdt_node_offset_by_compatible(fdt, -1,
			"nxp,s32g-ocotp");
	if (ocotp_node == -1)
		return;

	ret = s32cc_ocotp_init(fdt, ocotp_node);
	if (ret) {
		INFO("Failed to initialize OCOTP\n");
		return;
	}
}

void dt_init_wkpu(void)
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


	ret = s32cc_wkpu_init(fdt, wkpu_node);
	if (ret) {
		INFO("Failed to initialize WKPU\n");
		return;
	}
}

