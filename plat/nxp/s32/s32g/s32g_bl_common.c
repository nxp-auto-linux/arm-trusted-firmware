/*
 * Copyright 2020-2022 NXP
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
#include "s32_ncore.h"
#include "s32g_storage.h"
#include "s32g_bl_common.h"
#include "s32_dt.h"
#include "s32g_pinctrl.h"

#define S32G_MAX_I2C_MODULES 5

static struct s32g_i2c_driver i2c_drivers[S32G_MAX_I2C_MODULES];
static size_t i2c_fill_level;

struct s32g_i2c_driver *s32g_add_i2c_module(void *fdt, int fdt_node)
{
	struct s32g_i2c_driver *driver;
	struct dt_node_info i2c_info;
	size_t i;
	int ret;

	ret = fdt_node_check_compatible(fdt, fdt_node, "fsl,vf610-i2c");
	if (ret)
		return NULL;

	for (i = 0; i < i2c_fill_level; i++) {
		if (i2c_drivers[i].fdt_node == fdt_node)
			return &i2c_drivers[i];
	}

	if (i2c_fill_level >= ARRAY_SIZE(i2c_drivers)) {
		INFO("Discovered too many instances of I2C\n");
		return NULL;
	}

	driver = &i2c_drivers[i2c_fill_level];

	dt_fill_device_info(&i2c_info, fdt_node);

	if (i2c_info.base == 0U) {
		INFO("ERROR i2c base\n");
		return NULL;
	}

	driver->fdt_node = fdt_node;
	s32g_i2c_get_setup_from_fdt(fdt, fdt_node, &driver->bus);

	i2c_fill_level++;
	return driver;
}

void s32g_reinit_i2c(void)
{
	size_t i;

	i2c_config_pinctrl();

	for (i = 0; i < i2c_fill_level; i++)
		s32g_i2c_init(&i2c_drivers[i].bus);
}

void dt_init_pmic(void)
{
	void *fdt = NULL;
	int pmic_node;
	int i2c_node;
	struct s32g_i2c_driver *i2c_driver;
	int ret;

	if (dt_open_and_check() < 0) {
		INFO("ERROR fdt check\n");
		return;
	}

	if (fdt_get_address(&fdt) == 0) {
		INFO("ERROR fdt\n");
		return;
	}

	pmic_node = -1;
	while (true) {
		pmic_node = fdt_node_offset_by_compatible(fdt, pmic_node,
				"fsl,vr5510");
		if (pmic_node == -1)
			break;

		i2c_node = fdt_parent_offset(fdt, pmic_node);
		if (i2c_node == -1) {
			INFO("Failed to get parent of PMIC node\n");
			return;
		}

		i2c_driver = s32g_add_i2c_module(fdt, i2c_node);
		if (i2c_driver == NULL) {
			INFO("PMIC isn't subnode of an I2C node\n");
			return;
		}

		ret = vr5510_register_instance(fdt, pmic_node,
					       &i2c_driver->bus);
		if (ret) {
			INFO("Failed to register VR5510 instance\n");
			return;
		}
	}
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
			"fsl,s32g-ocotp");
	if (ocotp_node == -1)
		return;

	ret = s32gen1_ocotp_init(fdt, ocotp_node);
	if (ret) {
		INFO("Failed to initialize OCOTP\n");
		return;
	}
}

