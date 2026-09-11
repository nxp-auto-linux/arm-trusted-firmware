/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <s32_scmi_dt.h>

/* Number of cells in a GIC interrupt specifier (type, number, flags). */
#define S32_SCMI_IRQ_CELL_SIZE	(3U)

int s32_scmi_read_notif_irq(const void *fdt, int node)
{
	const fdt32_t *irq_prop;
	int irq_num, len = 0;

	irq_prop = fdt_getprop(fdt, node, "nxp,notif-irq", &len);
	if (irq_prop == NULL) {
		ERROR("Missing 'nxp,notif-irq' property.\n");
		return -FDT_ERR_NOTFOUND;
	}

	if (len != (int)(S32_SCMI_IRQ_CELL_SIZE * sizeof(uint32_t))) {
		ERROR("Invalid 'nxp,notif-irq' property.\n");
		return -FDT_ERR_BADSTRUCTURE;
	}

	irq_num = fdt_read_gic_irq_cells(irq_prop, S32_SCMI_IRQ_CELL_SIZE);
	if (irq_num < 0) {
		ERROR("Invalid 'nxp,notif-irq' interrupt specifier.\n");
		return -FDT_ERR_BADVALUE;
	}

	return irq_num;
}
