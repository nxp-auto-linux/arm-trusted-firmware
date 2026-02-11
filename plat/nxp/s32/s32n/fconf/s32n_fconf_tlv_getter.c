/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libfdt.h>
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <s32n_fconf_tlv_getter.h>
#include <s32n_fconf_utils.h>

static const char *tlv_compatible = "nxp,s32n79-tlv";
struct tlv_config tlv_config = {0};

int fconf_populate_tlv_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	uintptr_t base = 0;
	size_t size = 0;
	int node;

	node = fconf_get_node_info(hw_conf_dtb, -1, tlv_compatible, &base, &size);
	if (node < 0) {
		ERROR("FCONF: Error retrieving %s node info\n", tlv_compatible);
		return node;
	}

	tlv_config.tlv_base = base;
	tlv_config.tlv_size = size;

	return 0;
}
