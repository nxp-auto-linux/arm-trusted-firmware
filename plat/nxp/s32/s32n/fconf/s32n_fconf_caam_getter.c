/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <libfdt.h>

#include <s32n_fconf_caam_getter.h>

/*
 * S32N7x has 2 CAAM instances, each with 8 Job Rings (JRs).
 * Only a single JR is used by TF-A: the first JR node found enabled for
 * secure world (explicitly or implicitly, per the "secure-status" device
 * tree binding). This matches MAX_SEC_JOB_RINGS == 1 in the CAAM driver.
 */
#define CAAM_JR_COMPATIBLE	"fsl,sec-v4.0-job-ring"

/*
 * The index of a CAAM instance is given by the device tree alias ("caam0",
 * "caam1") pointing to its node, under the "/aliases" node.
 */
#define CAAM_ALIAS_PREFIX	"caam"
#define CAAM_MAX_INSTANCES	2U

struct caam_config caam_config = {0};

/*
 * Get the index of a CAAM instance, by looking for the "caam<N>" alias that
 * points to its node.
 */
static int caam_instance_from_alias(const void *dtb, int caam_node,
				    unsigned int *instance)
{
	char alias[sizeof(CAAM_ALIAS_PREFIX "N")];
	unsigned int i;

	for (i = 0U; i < CAAM_MAX_INSTANCES; i++) {
		(void)snprintf(alias, sizeof(alias), CAAM_ALIAS_PREFIX "%u", i);
		if (fdt_path_offset(dtb, alias) == caam_node) {
			*instance = i;
			return 0;
		}
	}

	return -ENODEV;
}

int fconf_populate_caam_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	int jr_node, caam_node = -1, ret;
	uintptr_t jr_base = 0, caam_base = 0;
	size_t jr_size = 0, caam_size = 0;
	uint64_t translated_addr;
	bool found = false;

	fdt_for_each_compatible_node(hw_conf_dtb, jr_node, CAAM_JR_COMPATIBLE) {
		if (!fdt_node_is_enabled_secure(hw_conf_dtb, jr_node)) {
			continue;
		}

		if (found) {
			WARN("FCONF: Multiple secure CAAM Job Rings found in dtb, only one is supported; ignoring %s\n",
			     fdt_get_name(hw_conf_dtb, jr_node, NULL));
			continue;
		}

		ret = fdt_get_reg_props_by_index(hw_conf_dtb, jr_node, 0,
						 &jr_base, &jr_size);
		if (ret != 0) {
			ERROR("FCONF: Failed to read 'reg' property of %s\n",
			      fdt_get_name(hw_conf_dtb, jr_node, NULL));
			return ret;
		}

		caam_node = fdt_parent_offset(hw_conf_dtb, jr_node);
		if (caam_node < 0) {
			ERROR("FCONF: Failed to get parent node of %s\n",
			      fdt_get_name(hw_conf_dtb, jr_node, NULL));
			return caam_node;
		}

		ret = fdt_get_reg_props_by_index(hw_conf_dtb, caam_node, 0,
						 &caam_base, &caam_size);
		if (ret != 0) {
			ERROR("FCONF: Failed to read 'reg' property of %s\n",
			      fdt_get_name(hw_conf_dtb, caam_node, NULL));
			return ret;
		}

		translated_addr = fdtw_translate_address(hw_conf_dtb, jr_node,
							 jr_base);
		if (translated_addr == ILLEGAL_ADDR) {
			ERROR("FCONF: Failed to translate address of %s\n",
			      fdt_get_name(hw_conf_dtb, jr_node, NULL));
			return -EINVAL;
		}
		jr_base = (uintptr_t)translated_addr;

		found = true;
	}

	if (!found) {
		WARN("FCONF: Can't find a secure CAAM Job Ring node in dtb\n");
		return 0;
	}

	ret = caam_instance_from_alias(hw_conf_dtb, caam_node,
				       &caam_config.instance);
	if (ret != 0) {
		ERROR("FCONF: Can't find a '%s<N>' alias for CAAM node %s\n",
		      CAAM_ALIAS_PREFIX,
		      fdt_get_name(hw_conf_dtb, caam_node, NULL));
		return ret;
	}

	caam_config.caam_base = caam_base;
	caam_config.caam_size = caam_size;
	caam_config.jr_base = jr_base;
	caam_config.jr_size = jr_size;

	INFO("FCONF: CAAM%u Job Ring @ 0x%lx (size 0x%lx)\n",
	     caam_config.instance, caam_config.jr_base, caam_config.jr_size);

	return 0;
}
