/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32_SCMI_DT_H
#define S32_SCMI_DT_H

/* Read the nxp,notif-irq GIC interrupt of a device tree node. */
int s32_scmi_read_notif_irq(const void *fdt, int node);

#endif /* S32_SCMI_DT_H */
