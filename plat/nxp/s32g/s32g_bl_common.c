/*
 * Copyright 2020-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <s32g_dt.h>
#include <common/debug.h>
#include <lib/mmio.h>
#include <libfdt.h>
#include <drivers/generic_delay_timer.h>
#include <drivers/nxp/s32g/pmic/vr5510.h>
#include <drivers/nxp/s32g/ocotp.h>
#include <lib/utils_def.h>
#include "platform_def.h"
#include "s32g_pinctrl.h"
#include "s32g_clocks.h"
#include "s32g_ncore.h"
#include "s32g_storage.h"
#include "s32g_bl_common.h"
#include "s32g_dt.h"
#include "s32g_pinctrl.h"

#define S32G_MAX_I2C_MODULES 5

static struct s32g_i2c_driver i2c_drivers[S32G_MAX_I2C_MODULES];
static size_t i2c_fill_level;

unsigned int plat_get_syscnt_freq2(void)
{
	return COUNTER_FREQUENCY;
}

void s32g_early_plat_init(bool skip_ddr_clk)
{
	uint32_t caiutc;

	s32g_plat_config_pinctrl();
	s32g_plat_clock_init(skip_ddr_clk);

	/* Restore (clear) the CAIUTC[IsolEn] bit for the primay cluster, which
	 * we have manually set during early BL2 boot.
	 */
	caiutc = mmio_read_32(S32G_NCORE_CAIU0_BASE_ADDR + NCORE_CAIUTC_OFF);
	caiutc &= ~NCORE_CAIUTC_ISOLEN_MASK;
	mmio_write_32(S32G_NCORE_CAIU0_BASE_ADDR + NCORE_CAIUTC_OFF, caiutc);

	ncore_init();
	ncore_caiu_online(A53_CLUSTER0_CAIU);

	generic_delay_timer_init();
}

void plat_ea_handler(unsigned int ea_reason, uint64_t syndrome, void *cookie,
		void *handle, uint64_t flags)
{
	ERROR("Unhandled External Abort received on 0x%lx at EL3!\n",
	      read_mpidr_el1());
	ERROR(" exception reason=%u syndrome=0x%llx\n", ea_reason, syndrome);

	panic();
}

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
		INFO("Failed to initialize WKPU\n");
		return;
	}
}

