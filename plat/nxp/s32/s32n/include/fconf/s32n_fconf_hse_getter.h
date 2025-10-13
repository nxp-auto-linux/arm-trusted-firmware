/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FCONF_HSE_GETTER_H
#define FCONF_HSE_GETTER_H

#include <lib/fconf/fconf.h>

#define hw_config__hse_config_getter(prop) hse_config.prop

struct hse_config_t {
	uintptr_t mu_base;
	size_t mu_size;
	uintptr_t rmem_base;
	size_t rmem_size;
};

extern struct hse_config_t hse_config;

int fconf_populate_hse_config(uintptr_t config);

#endif /* FCONF_HSE_GETTER_H */
