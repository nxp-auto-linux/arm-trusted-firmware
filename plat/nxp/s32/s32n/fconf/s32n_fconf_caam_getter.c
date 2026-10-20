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

/*
 * Device tree property describing the CAAM Secure Memory window owned by a
 * given Job Ring: <offset size>, both relative to the CAAM Secure Memory
 * base address. This is a fixed SoC/HSE2-level property (identical across
 * hw-partitions/cohorts/use cases), enforced by PaCo (part of the FSS
 * firmware) when validating a HSE2 "MSC Keys Push" service request.
 */
#define CAAM_SECMEM_RANGE_PROP	"nxp,secmem-range"
#define CAAM_SECMEM_RANGE_CELLS	2U

/* Total size of the CAAM Secure Memory, shared by all Job Rings */
#define CAAM_SM_TOTAL_SIZE	0x10000UL
/* CAAM Secure Memory page size; offset/size must be page-aligned */
#define CAAM_SM_PAGE_SIZE	0x1000UL

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

static int get_jr_secmem_range(const void *dtb, int jr_node,
			       size_t *sm_offset, size_t *sm_size)
{
	uint32_t secmem_range[CAAM_SECMEM_RANGE_CELLS];
	int ret;

	ret = fdt_read_uint32_array(dtb, jr_node, CAAM_SECMEM_RANGE_PROP,
				    CAAM_SECMEM_RANGE_CELLS, secmem_range);
	if (ret != 0) {
		ERROR("FCONF: Missing or invalid '%s' property of %s; the secure boot flow relies on CAAM Secure Memory to hold the signing key, so this is a fatal error\n",
		      CAAM_SECMEM_RANGE_PROP, fdt_get_name(dtb, jr_node, NULL));
		return ret;
	}

	*sm_offset = (size_t)secmem_range[0];
	*sm_size = (size_t)secmem_range[1];

	if ((*sm_offset % CAAM_SM_PAGE_SIZE) != 0 ||
	    (*sm_size % CAAM_SM_PAGE_SIZE) != 0 || *sm_size == 0 ||
	    *sm_offset + *sm_size > CAAM_SM_TOTAL_SIZE) {
		ERROR("FCONF: Invalid '%s' property of %s: offset 0x%zx, size 0x%zx\n",
		      CAAM_SECMEM_RANGE_PROP, fdt_get_name(dtb, jr_node, NULL),
		      *sm_offset, *sm_size);
		return -EINVAL;
	}

	return 0;
}

int fconf_populate_caam_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	int jr_node, caam_node = -1, ret;
	uintptr_t jr_base = 0;
	size_t jr_size = 0;
	size_t sm_offset = 0, sm_size = 0;
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

		translated_addr = fdtw_translate_address(hw_conf_dtb, jr_node,
							 jr_base);
		if (translated_addr == ILLEGAL_ADDR) {
			ERROR("FCONF: Failed to translate address of %s\n",
			      fdt_get_name(hw_conf_dtb, jr_node, NULL));
			return -EINVAL;
		}
		jr_base = (uintptr_t)translated_addr;

		ret = get_jr_secmem_range(hw_conf_dtb, jr_node, &sm_offset,
					  &sm_size);
		if (ret != 0) {
			return ret;
		}

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

	caam_config.jr_base = jr_base;
	caam_config.jr_size = jr_size;
	caam_config.sm_offset = sm_offset;
	caam_config.sm_size = sm_size;

	INFO("FCONF: CAAM%u Job Ring @ 0x%lx (size 0x%lx), Secure Memory window @ 0x%zx (size 0x%zx)\n",
	     caam_config.instance, caam_config.jr_base, caam_config.jr_size,
	     caam_config.sm_offset, caam_config.sm_size);

	return 0;
}
