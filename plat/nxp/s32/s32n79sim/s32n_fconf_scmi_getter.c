/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <s32n_fconf_scmi_getter.h>

/**
 * Update the macros in this file to customize the SCMI communication
 * parameters. The configuration will be retrieved through the FCONF
 * layer.
 */

/* SCMI OSPM channel default configuration. */
#define S32N_OSPM_SCMI_MEM		(0xd0000000)
#define S32N_OSPM_SCMI_MEM_SIZE	(0x80)

/**
 * Do not use a populate function, as the properties
 * are not read from the device tree.
 */
struct scmi_ospm_channel_config scmi_ospm_ch_config = {
	.ospm_mem_base = S32N_OSPM_SCMI_MEM,
	.ospm_mem_size = S32N_OSPM_SCMI_MEM_SIZE,
};

struct scmi_sh_mem_config scmi_sh_mem_config = {0};
struct scmi_fss_channel_config scmi_fss_ch_config = {0};
