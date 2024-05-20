/*
 * Copyright 2020-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef CLK_H
#define CLK_H

#include <stddef.h>
#include <stdint.h>
#include <s32cc_dt.h>

/* Assumption: Clock cells = 1*/
#define FDT_CLOCK_CELL_SIZE	(2 * sizeof(uint32_t))
#define CLK_DRV_NAME_SIZE	16

struct clk {
	struct clk_driver *drv;
	void *data;
	uint32_t id;
};

struct clk_ops {
	int (*enable)(struct clk *c, int enable);
	unsigned long (*set_rate)(struct clk *c, unsigned long rate);
	unsigned long (*get_rate)(struct clk *c);
	int (*set_parent)(struct clk *c, struct clk *parent);
	int (*request)(uint32_t id, struct clk *c);
};

struct clk_driver {
	const struct clk_ops *ops;
	void *data;
	uint32_t phandle;
	char name[CLK_DRV_NAME_SIZE];
};

static inline struct clk_driver *clk2clk_drv(struct clk *clk)
{
	return clk->drv;
}

static inline void *get_clk_drv_data(struct clk_driver *drv)
{
	return drv->data;
}

static inline unsigned long clk_get_rate(struct clk *c)
{
	return c->drv->ops->get_rate(c);
}

static inline int clk_enable(struct clk *c)
{
	return c->drv->ops->enable(c, 1);
}

void dt_clk_init(void);
int dt_clk_apply_defaults(void *fdt, int node);

void set_clk_driver_name(struct clk_driver *drv, const char *name);

/* Internal functions */
int dt_init_fixed_clk(void *fdt);
int dt_init_plat_clk(void *fdt);

struct clk_driver *allocate_clk_driver(void);
struct clk_driver *get_clk_driver(uint32_t phandle);
struct clk_driver *get_clk_driver_by_name(const char *name);
struct clk *allocate_clk(void);

int get_clk(uint32_t drv_id, uint32_t clk_id, struct clk *clock);
void init_fake_plat_driver(void *priv, const char *name);
int mmap_clk_regions(void *priv);

#endif
