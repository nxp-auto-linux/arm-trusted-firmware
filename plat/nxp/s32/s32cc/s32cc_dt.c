/*
 * Copyright (c) 2017-2019, ARM Limited and Contributors. All rights reserved.
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <clk/clk.h>
#include <inttypes.h>
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <drivers/arm/gic_common.h>
#include <errno.h>
#include <libfdt.h>
#include <platform_def.h>
#include <s32cc_bl_common.h>
#include <s32cc_dt.h>

static int fdt_checked;

static void *get_fdt(void)
{
	return (void *)get_bl2_dtb_base();
}

int dt_open_and_check(void)
{
	int ret = fdt_check_header(get_fdt());

	if (ret == 0)
		fdt_checked = 1;

	return ret;
}

int fdt_get_address(void **fdt_addr)
{
	if (fdt_checked == 1)
		*fdt_addr = get_fdt();

	return fdt_checked;
}

uint8_t fdt_get_status(int node)
{
	uint8_t status = DT_DISABLED;
	int len;
	const char *cchar;

	cchar = fdt_getprop(get_fdt(), node, "status", &len);
	if ((cchar == NULL) ||
			(strncmp(cchar, "okay", (size_t)len) == 0)) {
		status = DT_ENABLED;
	}

	return status;
}

int dt_enable_clocks(void *fdt_addr, int node)
{
	const fdt32_t *cuint;
	int len = 0, ret;
	uint32_t clk_id, clk_drv_id;
	size_t ncells, i, index;
	struct clk clk;

	cuint = fdt_getprop(fdt_addr, node, "clocks", &len);
	if (!cuint || len <= 0)
		return 0;

	ncells = (size_t)len / FDT_CLOCK_CELL_SIZE;
	// Invalid data
	if (!ncells)
		return 0;

	if (len % FDT_CLOCK_CELL_SIZE) {
		ERROR("Invalid clock definition for node: '%s'\n",
		      fdt_get_name(fdt_addr, node, NULL));
		return -EIO;
	}

	for (i = 0; i < ncells; i++) {
		index = i * 2;

		clk_drv_id = fdt32_to_cpu(cuint[index]);
		clk_id = fdt32_to_cpu(cuint[index + 1]);

		ret = get_clk(clk_drv_id, clk_id, &clk);
		if (ret) {
			ERROR("Failed to get the clock (drv:%" PRIu32 " clk%"
			      PRIu32 ") of the node '%s'\n",
			      clk_drv_id, clk_id,
			      fdt_get_name(fdt_addr, node, NULL));
			return ret;
		}

		ret = clk_enable(&clk);
		if (ret) {
			ERROR("Failed to enable the clock (drv:%" PRIu32
			      " clk:%" PRIu32 ") of the node '%s'\n",
			      clk_drv_id, clk_id,
			      fdt_get_name(fdt_addr, node, NULL));
			return ret;
		}
	}

	return 0;
}

void dt_fill_device_info(struct dt_node_info *info, int node)
{
	const fdt32_t *cuint;

	(void) fdt_get_reg_props_by_index(get_fdt(), node, 0,
					  &info->base, &info->reg_size);

	cuint = fdt_getprop(get_fdt(), node, "resets", NULL);
	if (cuint != NULL) {
		cuint++;
		info->reset = (int)fdt32_to_cpu(*cuint);
	} else
		info->reset = -1;

	info->status = fdt_get_status(node);
}

int fdt_read_irq_cells(const fdt32_t *prop, int nr_cells)
{
	int it_num;
	uint32_t res;

	if (!prop || nr_cells < 2)
		return -1;

	res = fdt32_to_cpu(prop[1]);
	if (res > MAX_SPI_ID)
		return -1;

	it_num = (int)res;

	switch (fdt32_to_cpu(prop[0])) {
	case 1:
		it_num += 16;
		break;
	case 0:
		it_num += 32;
		break;
	default:
		it_num = -1;
	}

	return it_num;
}

int fdt_get_irq_props_by_index(const void *dtb, int node,
			unsigned int irq_cell_size, unsigned int index, int *irq_num)
{
	const fdt32_t *prop;
	int parent, len = 0;
	uint32_t cell, res;

	if (!irq_cell_size) {
		/* if interrupt cell size is 0, then read it from device tree */
		parent = fdt_parent_offset(dtb, node);
		if (parent < 0)
			return -FDT_ERR_BADOFFSET;

		prop = fdt_getprop(dtb, parent, "#interrupt-cells", NULL);
		if (!prop) {
			INFO("Couldn't find \"#interrupts-cells\" property in dtb\n");
			return -FDT_ERR_NOTFOUND;
		}

		irq_cell_size = fdt32_to_cpu(*prop);
		if (irq_cell_size == 0)
			return -FDT_ERR_BADVALUE;
	}

	if (index > UINT32_MAX / irq_cell_size)
		return -FDT_ERR_BADVALUE;
	cell = index * irq_cell_size;

	prop = fdt_getprop(dtb, node, "interrupts", &len);
	if (!prop) {
		INFO("Couldn't find \"interrupts\" property in dtb\n");
		return -FDT_ERR_NOTFOUND;
	}

	if (cell > UINT32_MAX - irq_cell_size)
		return -FDT_ERR_BADVALUE;
	res = cell + irq_cell_size;

	if (res > UINT32_MAX / sizeof(uint32_t))
		return -FDT_ERR_BADVALUE;
	res = res * sizeof(uint32_t);

	if (res > (unsigned int)len)
		return -FDT_ERR_BADVALUE;

	if (irq_num) {
		*irq_num = fdt_read_irq_cells(&prop[cell], irq_cell_size);
		if (*irq_num < 0)
			return -FDT_ERR_BADVALUE;
	}

	return 0;
}

int fdt_node_offset_by_prop_found(const void *fdt, int startoffset,
				  const char *propname)
{
	int offset;
	const void *val;
	int len;

	for (offset = fdt_next_node(fdt, startoffset, NULL);
	     offset >= 0;
	     offset = fdt_next_node(fdt, offset, NULL)) {
		val = fdt_getprop(fdt, offset, propname, &len);
		if (val)
			return offset;
	}

	return offset; /* error from fdt_next_node() */
}
