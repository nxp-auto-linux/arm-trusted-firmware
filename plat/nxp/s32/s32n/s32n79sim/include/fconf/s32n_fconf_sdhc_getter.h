/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FCONF_SDHC_GETTER_H
#define FCONF_SDHC_GETTER_H

#include <lib/fconf/fconf.h>

#define hw_config__sdhc_config_getter(prop) sdhc_config.prop

struct sdhc_config_t {
	uint32_t per_clock_id;
};

extern struct sdhc_config_t sdhc_config;

int fconf_populate_sdhc_config(uintptr_t config);

#endif /* FCONF_SDHC_GETTER_H */
