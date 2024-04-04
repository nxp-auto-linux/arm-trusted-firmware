/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <lib/mmio.h>
#include <libfdt.h>
#include <drivers/nxp/s32/pmic/vr5510.h>
#include <drivers/nxp/s32/ocotp.h>
#include <lib/utils_def.h>
#include "platform_def.h"
#include "s32g_pinctrl.h"
#include "s32g_clocks.h"
#include "s32g_bl_common.h"
#include "s32cc_dt.h"
#include "s32g_pinctrl.h"
#include "s32gen1-wkpu.h"

extern struct s32_i2c_driver i2c_drivers[];
extern size_t i2c_fill_level;

void s32g_reinit_i2c(void)
{
	size_t i;

	i2c_config_pinctrl();

	for (i = 0; i < i2c_fill_level; i++)
		s32_i2c_init(&i2c_drivers[i].bus);
}

void dt_init_pmic(void)
{
	void *fdt = NULL;
	int pmic_node;
	int i2c_node;
	struct s32_i2c_driver *i2c_driver;
	unsigned int instance;
	int ret;

	ret = dt_open_and_check();
	if (ret < 0) {
		ERROR("ERROR fdt check\n");
		goto exit;
	}

	if (fdt_get_address(&fdt) == 0) {
		ret = -EIO;
		ERROR("ERROR fdt\n");
		goto exit;
	}

	pmic_node = -1;
	/* Limit the search to VR5510 MU & FSU */
	for (instance = 0u; instance < 2u; instance++) {
		pmic_node = fdt_node_offset_by_compatible(fdt, pmic_node,
				"nxp,vr5510");
		if (pmic_node == -1) {
			ret = -EIO;
			ERROR("Failed to get VR5510 instance %u\n", instance);
			break;
		}

		i2c_node = fdt_parent_offset(fdt, pmic_node);
		if (i2c_node == -1) {
			ret = -EIO;
			ERROR("Failed to get the parent of VR5510 %u node\n",
			      instance);
			break;
		}

		i2c_driver = s32_add_i2c_module(fdt, i2c_node);
		if (i2c_driver == NULL) {
			ret = -EIO;
			ERROR("VR5510 %u isn't subnode of an I2C node\n",
			      instance);
			break;
		}

		ret = vr5510_register_instance(fdt, pmic_node,
					       &i2c_driver->bus);
		if (ret) {
			ret = -EIO;
			ERROR("Failed to register VR5510 instance %u\n",
			      instance);
			break;
		}
	}
exit:
	if (ret)
		panic();
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

	ret = s32gen1_ocotp_init(fdt, ocotp_node);
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


	ret = s32gen1_wkpu_init(fdt, wkpu_node);
	if (ret) {
		INFO("Failed to initialize WKPU\n");
		return;
	}
}

