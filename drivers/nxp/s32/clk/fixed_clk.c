/*
 * Copyright 2020,2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <clk/clk.h>
#include <common/debug.h>
#include <errno.h>
#include <libfdt.h>
#include <libfdt_env.h>
#include <memory_pool.h>

#define NUM_FIXED_DRVS	23

struct fixed_clk_drv {
	unsigned long freq;
	const char *name;
};

static unsigned long fixed_clk_get_rate(struct clk *c);
static int fixed_clk_request(uint32_t id, struct clk *c);

static struct fixed_clk_drv fixed_drvs[NUM_FIXED_DRVS];
static struct memory_pool pool = INIT_MEM_POOL(fixed_drvs);

static const struct clk_ops fixed_clk_ops = {
	.get_rate = fixed_clk_get_rate,
	.request = fixed_clk_request,
};

static int fixed_clk_request(uint32_t id, struct clk *c)
{
	return 0;
}

static unsigned long fixed_clk_get_rate(struct clk *c)
{
	struct clk_driver *drv = c->drv;
	struct fixed_clk_drv *fix = drv->data;

	return fix->freq;
}

static int register_fixed_clk(uint32_t phandle, const char *name, uint32_t freq)
{
	struct fixed_clk_drv *fix;
	struct clk_driver *drv;

	drv = allocate_clk_driver();
	if (!drv) {
		ERROR("Failed to allocate a clock driver\n");
		return -ENOMEM;
	}

	fix = alloc_mem_pool_elem(&pool);
	if (!fix) {
		ERROR("Failed to allocate a fixed clock driver\n");
		return -ENOMEM;
	}

	fix->name = name;
	fix->freq = freq;

	drv->ops = &fixed_clk_ops;
	drv->phandle = phandle;
	drv->data = fix;

	set_clk_driver_name(drv, name);

	return 0;
}

int dt_init_fixed_clk(void *fdt)
{
	int node, len, ret;
	const fdt32_t *clk_freq_prop;
	const char *name;
	uint32_t freq, phandle;

	node = -1;
	while (true) {
		node = fdt_node_offset_by_compatible(fdt, node,
						     "fixed-clock");
		if (node == -1)
			break;

		clk_freq_prop = fdt_getprop(fdt, node, "clock-frequency", &len);
		if (!clk_freq_prop) {
			ERROR("'clock-frequency' property is mandatory\n");
			return len;
		}

		phandle = fdt_get_phandle(fdt, node);
		freq = fdt32_to_cpu(*clk_freq_prop);

		name = fdt_get_name(fdt, node, NULL);
		ret = register_fixed_clk(phandle, name, freq);

		if (ret) {
			ERROR("Failed to register a fixed clock\n");
			return ret;
		}
	}

	return 0;
}


