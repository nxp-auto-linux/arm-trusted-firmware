/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <s32n_fconf_scmi_getter.h>

/**
 * Update the macros in this file to customize the SCMI communication
 * parameters. The configuration will be retrieved through the FCONF
 * layer.
 */

/* Default FSS mailbox addresses */
#define FSS_SCMI_SH_MEM_0	(0x2483A000)
#define FSS_SCMI_SH_MEM_1	(0x2483B000)
#define FSS_SCMI_SH_MEM_2	(0x2483C000)
#define FSS_SCMI_SH_MEM_3	(0x2483D000)

#define S32N_FSS_SCMI_SH_MEM_SIZE	(4U * 1024U)

/* Default mailbox configurations */
#if (S32N_SCMI_MBX == 0)
#define S32N_FSS_SCMI_SH_MEM		FSS_SCMI_SH_MEM_0
#define FSS_MRU_CH                  (3)
#elif (S32N_SCMI_MBX == 1)
#define S32N_FSS_SCMI_SH_MEM		FSS_SCMI_SH_MEM_1
#define FSS_MRU_CH                  (4)
#elif (S32N_SCMI_MBX == 2)
#define S32N_FSS_SCMI_SH_MEM		FSS_SCMI_SH_MEM_2
#define FSS_MRU_CH                  (5)
#elif (S32N_SCMI_MBX == 3)
#define S32N_FSS_SCMI_SH_MEM		FSS_SCMI_SH_MEM_3
#define FSS_MRU_CH                  (6)
#else
#error Invalid SCMI mailbox configuration!
#endif

#define S32N_SCMI_MBX_ADDR		(S32N_FSS_SCMI_SH_MEM + 0x100)
#define S32N_SCMI_MBX_SIZE		(0x80)

/* Used in the handshake procedure */
#define S32N_FSS_REMOTE_HANDSHAKE	(S32N_FSS_SCMI_SH_MEM + 0x0)
#define S32N_FSS_LOCAL_HANDSHAKE	(S32N_FSS_SCMI_SH_MEM + 0x40)

/**
 * Do not use a populate function, as the properties
 * are not read from the device tree.
 */
struct scmi_ospm_channel_config scmi_ospm_ch_config = {0};

struct scmi_sh_mem_config scmi_sh_mem_config = {
	.sh_mem_base = S32N_FSS_SCMI_SH_MEM,
	.sh_mem_size = S32N_FSS_SCMI_SH_MEM_SIZE,
	.remote_handshake_addr = S32N_FSS_REMOTE_HANDSHAKE,
	.local_handshake_addr = S32N_FSS_LOCAL_HANDSHAKE,
};

struct scmi_fss_channel_config scmi_fss_ch_config = {
	.mb_base = S32N_SCMI_MBX_ADDR,
	.mb_size = S32N_SCMI_MBX_SIZE,
	.mru_ch = FSS_MRU_CH,
};

static const char *scmi_ospm_compatible = "arm,scmi-shmem";

int fconf_populate_scmi_ospm_ch_config(uintptr_t config)
{
	const void *hw_conf_dtb = (const void *)config;
	int scmi_ospm_node, ret;
	uintptr_t base = 0;
	size_t size = 0;

	fdt_for_each_compatible_node(hw_conf_dtb, scmi_ospm_node, scmi_ospm_compatible) {
		if (!fdt_node_is_enabled(hw_conf_dtb, scmi_ospm_node)) {
			continue;
		}

		ret = fdt_get_reg_props_by_index(hw_conf_dtb, scmi_ospm_node, 0,
						 &base, &size);
		if (ret)
			continue;

		scmi_ospm_ch_config.ospm_mem_base = base;
		scmi_ospm_ch_config.ospm_mem_size = size;
		return 0;
	}

	ERROR("FCONF: Can't find %s compatible node in dtb\n", scmi_ospm_compatible);
	return -FDT_ERR_BADSTRUCTURE;
}

FCONF_REGISTER_POPULATOR(HW_CONFIG, scmi_ospm_ch_config, fconf_populate_scmi_ospm_ch_config);
