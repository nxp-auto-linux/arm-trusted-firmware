/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <libfdt.h>
#include <platform_def.h>
#include <s32n_bl_common.h>
#include <s32n_fconf_gic_getter.h>
#include <s32n_fss_scmi.h>

/**
 * GIC configurations control the mapping between
 * the APS clusters and the available GIC instances.
 */
#define GIC_DEDICATED_MODE	(0)
#define GIC_COMMON_MODE		(1)
#define GIC_TWO_HV_MODE		(2)
#define GIC_THREE_HV_MODE	(3)
#define GIC_SAFETY_MODE		(4)

/**
 * Update this define to change the GIC configuration mode.
 * This setting will be provided by FSS FW.
 */
#if (S32N_GIC_MODE == 0)
#define GIC_MODE			GIC_DEDICATED_MODE
#elif (S32N_GIC_MODE == 1)
#define GIC_MODE			GIC_COMMON_MODE
#elif (S32N_GIC_MODE == 2)
#define GIC_MODE			GIC_TWO_HV_MODE
#elif (S32N_GIC_MODE == 3)
#define GIC_MODE			GIC_THREE_HV_MODE
#elif (S32N_GIC_MODE == 4)
#define GIC_MODE			GIC_SAFETY_MODE
#else
#error Invalid GIC mode configuration!
#endif

/* 4 APS GIC instances are available */
#define GIC_MAX_INSTANCE	(4)

static const uintptr_t gic_instances[GIC_MAX_INSTANCE] = {
	PLAT_GICD_ADDR(0),
	PLAT_GICD_ADDR(1),
	PLAT_GICD_ADDR(2),
	PLAT_GICD_ADDR(3)
};

static const char *gicv3_compatible = "arm,gic-v3";
struct gic_config_t gic_config = {0};

static void populate_gic_instance(void)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(gic_instances); i++) {
		if (gic_config.gicd_base == gic_instances[i]) {
			gic_config.instance = i;
			return;
		}
	}
}

int fconf_populate_gic_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	int node, ret;
	uintptr_t base = 0;
	size_t size = 0;

	node = fdt_node_offset_by_compatible(hw_conf_dtb, -1, gicv3_compatible);
	if (node < 0) {
		ERROR("FCONF: Missing node with %s compatible.\n", gicv3_compatible);
		return 0;
	}

	ret = fdt_get_reg_props_by_index(hw_conf_dtb, node, 0, &base, &size);
	if (ret < 0) {
		ERROR("FCONF: Failed to read GICD reg property\n");
		return ret;
	}

	gic_config.gicd_base = base;
	gic_config.gicd_size = size;

	ret = fdt_get_reg_props_by_index(hw_conf_dtb, node, 1, &base, &size);
	if (ret < 0) {
		ERROR("FCONF: Failed to read GICR reg property\n");
		return ret;
	}

	gic_config.gicr_base = base;
	gic_config.gicr_size = size;

	populate_gic_instance();

	if (is_fss_used())
		return fss_get_gic_mode(&gic_config.gic_mode);

	/* Temporary support in case FSS is not enabled, default to
	 * GIC Mode 0 (Dedicated).
	 */
	gic_config.gic_mode = GIC_MODE;

	return 0;
}

FCONF_REGISTER_POPULATOR(HW_CONFIG, gic_config, fconf_populate_gic_config);
