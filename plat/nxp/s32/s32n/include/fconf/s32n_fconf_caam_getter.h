/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FCONF_CAAM_GETTER_H
#define FCONF_CAAM_GETTER_H

#include <lib/fconf/fconf.h>

#define hw_config__caam_config_getter(prop) caam_config.prop

/*
 * struct caam_config - CAAM configuration, as extracted from the device tree
 * @jr_base:	Absolute base address of the first Job Ring node found
 *		enabled for secure world (explicitly or implicitly, per the
 *		"secure-status" device tree binding).
 * @jr_size:	Size of the Job Ring's register block.
 * @instance:	Index (0 or 1) of the CAAM instance owning the selected Job
 *		Ring, as given by the "caam<N>" device tree alias pointing
 *		to the Job Ring's parent (CAAM instance) node.
 */
struct caam_config {
	uintptr_t jr_base;
	size_t jr_size;
	unsigned int instance;
};

extern struct caam_config caam_config;

int fconf_populate_caam_config(uintptr_t config);

#endif /* FCONF_CAAM_GETTER_H */
