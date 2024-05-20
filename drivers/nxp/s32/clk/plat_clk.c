/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <clk/clk.h>
#include <clk/s32gen1_clk_funcs.h>
#include <clk/s32gen1_clk_modules.h>
#include <clk/s32gen1_scmi_clk.h>
#include <libfdt.h>
#include <libfdt_env.h>
#include <s32cc_dt.h>
#include <s32cc_clocks.h>
#include "s32cc_mc_me.h"
#include "s32cc_mc_rgm.h"

struct s32gen1_clk_driver {
	struct s32gen1_clk_priv priv;
	struct clk_driver *driver;
};

struct s32gen1_clk_mappings {
	bool accelpll;
	bool armdfs;
	bool armpll;
	bool cgm0;
	bool cgm1;
	bool cgm2;
	bool cgm5;
	bool cgm6;
	bool ddrpll;
	bool fxosc;
	bool mc_me;
	bool periphdfs;
	bool periphpll;
	bool rdc;
	bool rgm;
};

int mmap_clk_regions(void *priv)
{
	static struct s32gen1_clk_mappings mappings;
	struct s32gen1_clk_priv *clk_priv = priv;
	unsigned long long reg_base;
	size_t reg_size, idx;
	int ret;

	struct clk_regs {
		void *reg_base;
		size_t reg_size;
		bool *mmap_added;
	} clks[] = {
		{
			.reg_base = clk_priv->fxosc,
			.reg_size = S32_FXOSC_SIZE,
			.mmap_added = &mappings.fxosc,
		},
		{
			.reg_base = clk_priv->cgm0,
			.reg_size = MC_CGM0_SIZE,
			.mmap_added = &mappings.cgm0,
		},
		{
			.reg_base = clk_priv->mc_me,
			.reg_size = S32_MC_ME_SIZE,
			.mmap_added = &mappings.mc_me,
		},
		{
			.reg_base = clk_priv->rdc,
			.reg_size = RDC_SIZE,
			.mmap_added = &mappings.rdc,
		},
		{
			.reg_base = clk_priv->rgm,
			.reg_size = S32_MC_RGM_SIZE,
			.mmap_added = &mappings.rgm,
		},
		{
			.reg_base = clk_priv->cgm1,
			.reg_size = MC_CGM1_SIZE,
			.mmap_added = &mappings.cgm1,
		},
		{
			.reg_base = clk_priv->cgm2,
			.reg_size = MC_CGM2_SIZE,
			.mmap_added = &mappings.cgm2,
		},
		{
			.reg_base = clk_priv->cgm5,
			.reg_size = MC_CGM5_SIZE,
			.mmap_added = &mappings.cgm5,
		},
#if defined(MC_CGM6_BASE_ADDR)
		{
			.reg_base = clk_priv->cgm6,
			.reg_size = MC_CGM6_SIZE,
			.mmap_added = &mappings.cgm6,
		},
#endif
		{
			.reg_base = clk_priv->armpll,
			.reg_size = ARM_PLL_SIZE,
			.mmap_added = &mappings.armpll,
		},
		{
			.reg_base = clk_priv->periphpll,
			.reg_size = PERIPH_PLL_SIZE,
			.mmap_added = &mappings.periphpll,
		},
		{
			.reg_base = clk_priv->accelpll,
			.reg_size = ACCEL_PLL_SIZE,
			.mmap_added = &mappings.accelpll,
		},
		{
			.reg_base = clk_priv->ddrpll,
			.reg_size = DRAM_PLL_SIZE,
			.mmap_added = &mappings.ddrpll,
		},
		{
			.reg_base = clk_priv->armdfs,
			.reg_size = ARM_DFS_SIZE,
			.mmap_added = &mappings.armdfs,
		},
		{
			.reg_base = clk_priv->periphdfs,
			.reg_size = PERIPH_DFS_SIZE,
			.mmap_added = &mappings.periphdfs,
		},
	};

	if (!is_mmu_el3_enabled())
		return 0;

	for (idx = 0; idx < ARRAY_SIZE(clks); idx++) {
		reg_base = (uint64_t)(uintptr_t)clks[idx].reg_base;
		reg_size = (size_t)round_up(clks[idx].reg_size, PAGE_SIZE);

		if (!reg_base)
			continue;

		ret = s32_mmap_dynamic_region(reg_base, reg_size, MT_DEVICE | MT_RW,
					      clks[idx].mmap_added);

		if (ret) {
			return ret;
		}
	}

	return 0;
}

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

	ret = mmap_clk_regions(priv);
	if (ret)
		return ret;

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

	ret = dt_enable_clocks(fdt, node);
	if (ret)
		return ret;

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
