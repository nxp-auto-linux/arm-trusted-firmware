/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FCONF_TLV_GETTER_H
#define FCONF_TLV_GETTER_H

#include <lib/fconf/fconf.h>

#define hw_config__tlv_config_getter(prop) tlv_config.prop

struct tlv_config {
	uintptr_t tlv_base;
	size_t tlv_size;
};

extern struct tlv_config tlv_config;

int fconf_populate_tlv_config(uintptr_t config);

#endif /* FCONF_TLV_GETTER_H */
