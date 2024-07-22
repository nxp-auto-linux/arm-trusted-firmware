/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <clk/clk.h>
#include <common/debug.h>
#include <errno.h>
#include <libfdt.h>
#include <memory_pool.h>
#include <plat/common/platform.h>
#include <s32cc_dt.h>
#include <inttypes.h>

/*
 * We use a clk_driver structure for each 'fixed-clock' node in the
 * device tree and one for 'nxp,s32cc-clocking' node.
 */
#define MAX_NUM_DRV 24

static struct clk_driver drivers[MAX_NUM_DRV];
static struct memory_pool drv_pool = INIT_MEM_POOL(drivers);

struct clk_driver *allocate_clk_driver(void)
{
	return alloc_mem_pool_elem(&drv_pool);
}

void set_clk_driver_name(struct clk_driver *drv, const char *name)
{
	size_t max_offset = sizeof(drv->name) - 1;
	size_t len = strnlen(name, max_offset);

	drv->name[len] = '\0';
	memcpy(drv->name, name, len);
}

struct clk_driver *get_clk_driver(uint32_t phandle)
{
	size_t i;

	if (!phandle)
		return NULL;

	for (i = 0; i < drv_pool.fill_level; i++) {
		if (drivers[i].phandle == phandle)
			return &drivers[i];
	}

	return NULL;
}

struct clk_driver *get_clk_driver_by_name(const char *name)
{
	size_t i;
	size_t len = strnlen(name, CLK_DRV_NAME_SIZE);

	if (!name)
		return NULL;

	if (len + 1 < CLK_DRV_NAME_SIZE)
		len++;

	for (i = 0; i < drv_pool.fill_level; i++) {
		if (!memcmp(name, drivers[i].name, len))
			return &drivers[i];
	}

	return NULL;
}

void dt_clk_init(void)
{
	int ret;
	void *fdt;

	if (dt_open_and_check() < 0) {
		ERROR("Invalid DTB\n");
		goto panic;
	}

	if (fdt_get_address(&fdt) == 0) {
		ERROR("Failed to get FDT address\n");
		goto panic;
	}

	ret = dt_init_fixed_clk(fdt);
	if (ret) {
		ERROR("Failed to initialize fixed clocks\n");
		goto panic;
	}

	ret = dt_init_plat_clk(fdt);
	if (ret) {
		ERROR("Failed to initialize platform clocks\n");
		goto panic;
	}

	return;
panic:
	plat_panic_handler();
}

int get_clk(uint32_t drv_id, uint32_t clk_id, struct clk *clock)
{
	struct clk_driver *clk_drv;

	clk_drv = get_clk_driver(drv_id);
	if (!clk_drv) {
		ERROR("Invalid clock driver ID: %" PRIu32 "\n", drv_id);
		return -EIO;
	}

	clock->drv = clk_drv;
	clock->data = NULL;
	clock->id = clk_id;

	if (clk_drv->ops->request)
		return clk_drv->ops->request(clk_id, clock);

	return 0;
}


static int process_parents_prop(size_t *parent_index, size_t nparents,
				const fdt32_t *parents, struct clk *clk,
				struct clk *parent_clk)
{
	uint32_t parent_drv_id, parent_clk_id;
	int ret;

	if (!parents)
		return 0;

	if (*parent_index >= nparents)
		return 0;

	parent_drv_id = fdt32_to_cpu(parents[*parent_index]);
	if (!parent_drv_id) {
		(*parent_index)++;
	} else {
		parent_clk_id = fdt32_to_cpu(parents[*parent_index + 1]);
		(*parent_index) += 2;

		ret = get_clk(parent_drv_id, parent_clk_id, parent_clk);
		if (ret) {
			ERROR("Unidentified clock id: %" PRIu32 "\n",
			      parent_clk_id);
			return -EIO;
		}

		if (clk->drv->ops && clk->drv->ops->set_parent) {
			ret = clk->drv->ops->set_parent(clk, parent_clk);
			if (ret) {
				ERROR("Failed to set parent of %" PRIu32 " clk\n",
						clk->id);
			}
			return ret;
		}
	}

	return 0;
}

int dt_clk_apply_defaults(void *fdt, int node)
{
	const fdt32_t *clocks, *parents, *rates;
	int nclocks_size, nparents_size, nrates_size;
	int ret, index;
	uint32_t clk_drv_id, clk_id, freq, i;
	struct clk clk, parent_clk;
	size_t nparents, parent_index;
	int fret = 0;

	clocks = fdt_getprop(fdt, node, "assigned-clocks", &nclocks_size);
	/* No clock settings to be applied */
	if (!clocks)
		return 0;

	parents = fdt_getprop(fdt, node, "assigned-clock-parents",
			      &nparents_size);

	rates = fdt_getprop(fdt, node, "assigned-clock-rates", &nrates_size);

	nparents = nparents_size / sizeof(uint32_t);

	parent_index = 0;
	for (i = 0; i < nclocks_size / FDT_CLOCK_CELL_SIZE; i++) {
		index = i * 2;

		clk_drv_id = fdt32_to_cpu(clocks[index]);
		clk_id = fdt32_to_cpu(clocks[index + 1]);
		if (!clk_drv_id) {
			ERROR("Invalid clock driver id: 0.\n");
			fret = -EIO;
			continue;
		}

		ret = get_clk(clk_drv_id, clk_id, &clk);
		if (ret) {
			fret = -EIO;
			continue;
		}

		ret = process_parents_prop(&parent_index, nparents,
					   parents, &clk, &parent_clk);
		if (ret) {
			ERROR("Failed to process parents field for '%s' node\n",
			      fdt_get_name(fdt, node, NULL));
			fret = -EINVAL;
			continue;
		}

		if (!rates)
			continue;

		if (i >= nrates_size / sizeof(uint32_t)) {
			ERROR("Failed to process rates field for '%s' node\n",
			      fdt_get_name(fdt, node, NULL));
			fret = -EINVAL;
			continue;
		}

		freq = fdt32_to_cpu(rates[i]);

		if (freq && clk.drv->ops && clk.drv->ops->set_rate) {
			ret = clk.drv->ops->set_rate(&clk, freq);
			if (!ret) {
				ERROR("Failed to set rate of %" PRIu32 " clk\n",
						clk.id);
				fret = ret;
				continue;
			}
		}

	}

	return fret;
}

void init_fake_plat_driver(void *priv, const char *name)
{
	static struct clk_driver *driver;

	if (!driver) {
		driver = allocate_clk_driver();
		driver->data = priv;

		set_clk_driver_name(driver, name);
	}
}
