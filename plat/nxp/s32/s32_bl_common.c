/*
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <drivers/generic_delay_timer.h>
#include <libfdt.h>
#include <lib/mmio.h>
#include "platform_def.h"
#include "s32_bl_common.h"
#include "s32_clocks.h"
#if (ERRATA_S32_050543 == 1)
#include <dt-bindings/ddr-errata/s32-ddr-errata.h>
#include "s32_ddr_errata_funcs.h"
#endif
#include "s32_dt.h"
#include "s32_ncore.h"
#include "s32_pinctrl.h"

struct s32_i2c_driver i2c_drivers[S32_MAX_I2C_MODULES];
size_t i2c_fill_level;

bool is_lockstep_enabled(void)
{
	if (mmio_read_32(GPR_BASE_ADDR + GPR06_OFF) & CA53_LOCKSTEP_EN)
		return true;

	return false;
}

#if (ERRATA_S32_050543 == 1)
void ddr_errata_update_flag(uint8_t flag)
{
	mmio_write_32(DDR_ERRATA_REGION_BASE, flag);
}
#endif

void s32_early_plat_init(bool skip_ddr_clk)
{
	uint32_t caiutc;

	s32_plat_config_pinctrl();
	s32_plat_clock_init(skip_ddr_clk);

	/* Restore (clear) the CAIUTC[IsolEn] bit for the primay cluster, which
	 * we have manually set during early BL2 boot.
	 */
	caiutc = mmio_read_32(S32_NCORE_CAIU0_BASE_ADDR + NCORE_CAIUTC_OFF);
	caiutc &= ~NCORE_CAIUTC_ISOLEN_MASK;
	mmio_write_32(S32_NCORE_CAIU0_BASE_ADDR + NCORE_CAIUTC_OFF, caiutc);

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

unsigned int plat_get_syscnt_freq2(void)
{
	return COUNTER_FREQUENCY;
}

struct s32_i2c_driver *s32_add_i2c_module(void *fdt, int fdt_node)
{
	struct s32_i2c_driver *driver;
	struct dt_node_info i2c_info;
	size_t i;
	int ret;

	ret = fdt_node_check_compatible(fdt, fdt_node, "nxp,s32cc-i2c");
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
	s32_i2c_get_setup_from_fdt(fdt, fdt_node, &driver->bus);

	i2c_fill_level++;
	return driver;
}

