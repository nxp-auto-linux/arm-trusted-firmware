/*
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <clk/clk.h>
#include <clk/s32gen1_clk_funcs.h>
#include <clk/s32gen1_clk_modules.h>
#include <clk/s32gen1_scmi_clk.h>
#include <libfdt.h>
#include <libfdt_env.h>
#include <s32_dt.h>

struct s32gen1_clk_driver {
	struct s32gen1_clk_priv priv;
	struct clk_driver *driver;
};

void *get_base_addr(enum s32gen1_clk_source id, struct s32gen1_clk_priv *priv)
{
	switch (id) {
	case S32GEN1_ACCEL_PLL:
		return priv->accelpll;
	case S32GEN1_ARM_DFS:
		return priv->armdfs;
	case S32GEN1_ARM_PLL:
		return priv->armpll;
	case S32GEN1_CGM0:
		return priv->cgm0;
	case S32GEN1_CGM1:
		return priv->cgm1;
	case S32GEN1_CGM2:
		return priv->cgm2;
	case S32GEN1_CGM5:
		return priv->cgm5;
	case S32GEN1_CGM6:
		return priv->cgm6;
	case S32GEN1_DDR_PLL:
		return priv->ddrpll;
	case S32GEN1_FXOSC:
		return priv->fxosc;
	case S32GEN1_PERIPH_DFS:
		return priv->periphdfs;
	case S32GEN1_PERIPH_PLL:
		return priv->periphpll;
	default:
		ERROR("Unknown clock source id: %u\n", id);
	}

	return NULL;
}

static int bind_clk_provider(struct s32gen1_clk_driver *drv, void *fdt,
			     const char *compatible, void **base_addr,
			     int *node)
{
	struct dt_node_info info;

	*node = fdt_node_offset_by_compatible(fdt, -1, compatible);
	if (*node == -1) {
		ERROR("Failed to get '%s' node\n", compatible);
		return -EIO;
	}

	dt_fill_device_info(&info, *node);
	if (!info.base) {
		ERROR("Invalid 'reg' property of %s node\n", compatible);
		return -EIO;
	}

	*base_addr = (void *)(uintptr_t)info.base;

	return dt_clk_apply_defaults(fdt, *node);
}

static int s32gen1_clk_probe(struct s32gen1_clk_driver *drv, void *fdt,
			     int node)
{
	struct s32gen1_clk_priv *priv = &drv->priv;
	int ret;
	size_t i;

	struct clk_dep {
		void **base_addr;
		const char *compat;
		int node;
	} deps[] = {
		{
			.base_addr = &priv->fxosc,
			.compat = "nxp,s32cc-fxosc",
		},
		{
			.base_addr = &priv->cgm0,
			.compat = "nxp,s32cc-mc_cgm0",
		},
		{
			.base_addr = &priv->mc_me,
			.compat = "nxp,s32cc-mc_me",
		},
		{
			.base_addr = &priv->rdc,
			.compat = "nxp,s32cc-rdc",
		},
		{
			.base_addr = &priv->rgm,
			.compat = "nxp,s32cc-rgm",
		},
		{
			.base_addr = &priv->cgm1,
			.compat = "nxp,s32cc-mc_cgm1",
		},
		{
			.base_addr = &priv->cgm2,
			.compat = "nxp,s32cc-mc_cgm2",
		},
		{
			.base_addr = &priv->cgm5,
			.compat = "nxp,s32cc-mc_cgm5",
		},
		{
			.base_addr = &priv->cgm6,
			.compat = "nxp,s32cc-mc_cgm6",
		},
		{
			.base_addr = &priv->armpll,
			.compat = "nxp,s32cc-armpll",
		},
		{
			.base_addr = &priv->periphpll,
			.compat = "nxp,s32cc-periphpll",
		},
		{
			.base_addr = &priv->accelpll,
			.compat = "nxp,s32cc-accelpll",
		},
		{
			.base_addr = &priv->ddrpll,
			.compat = "nxp,s32cc-ddrpll",
		},
		{
			.base_addr = &priv->armdfs,
			.compat = "nxp,s32cc-armdfs",
		},
		{
			.base_addr = &priv->periphdfs,
			.compat = "nxp,s32cc-periphdfs",
		},
	};

	for (i = 0; i < ARRAY_SIZE(deps); i++) {
		bind_clk_provider(drv, fdt, deps[i].compat,
				  deps[i].base_addr, &deps[i].node);
	}

	ret = dt_clk_apply_defaults(fdt, node);
	if (ret) {
		ERROR("Failed to apply default clocks for '%s'\n",
		      fdt_get_name(fdt, node, NULL));
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(deps); i++) {
		ret = dt_enable_clocks(fdt, deps[i].node);
		if (ret)
			return ret;
	}

	return 0;
}

static const struct clk_ops s32gen1_clk_ops = {
	.enable = s32gen1_scmi_enable,
	.set_rate = s32gen1_scmi_set_rate,
	.get_rate = s32gen1_scmi_get_rate,
	.set_parent = s32gen1_scmi_set_parent,
	.request = s32gen1_scmi_request,
};

int dt_init_plat_clk(void *fdt)
{
	static struct s32gen1_clk_driver clk_drv;
	int node;

	node = fdt_node_offset_by_compatible(fdt, -1, "nxp,s32cc-clocking");
	if (node == -1) {
		ERROR("Failed to detect S32-GEN1 clock compatible.\n");
		return -EIO;
	}

	clk_drv.driver = allocate_clk_driver();
	if (!clk_drv.driver) {
		ERROR("Failed to allocate clock driver\n");
		return -ENOMEM;
	}

	clk_drv.driver->ops = &s32gen1_clk_ops;
	clk_drv.driver->phandle = fdt_get_phandle(fdt, node);
	clk_drv.driver->data = &clk_drv;

	set_clk_driver_name(clk_drv.driver, fdt_get_name(fdt, node, NULL));

	return s32gen1_clk_probe(&clk_drv, fdt, node);
}
