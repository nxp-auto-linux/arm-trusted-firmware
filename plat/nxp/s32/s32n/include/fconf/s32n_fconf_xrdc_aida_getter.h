/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FCONF_XRDC_AIDA_GETTER_H
#define FCONF_XRDC_AIDA_GETTER_H

#include <lib/fconf/fconf.h>

#define hw_config__xrdc_aida_config_getter(prop) xrdc_aida_config.prop

struct xrdc_aida_config_entry {
	uintptr_t base_addr;
	uint32_t stream_id;
};

struct xrdc_aida_config {
	uint32_t entries_n_elem;
	struct xrdc_aida_config_entry *entries;
};

extern struct xrdc_aida_config xrdc_aida_config;

int fconf_populate_xrdc_aida_config(uintptr_t config);

#endif /* FCONF_XRDC_AIDA_GETTER_H */
