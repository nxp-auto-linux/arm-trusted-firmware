/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <plat/common/common_def.h>
#include <s32n_fconf_scmi_getter.h>

/**
 * Update the macros in this file to customize the SCMI communication
 * parameters. The configuration will be retrieved through the FCONF
 * layer.
 */

/* SCMI OSPM channel default configuration. */
#define S32N_OSPM_SCMI_MEM		(0xd0000000)
#define S32N_OSPM_SCMI_MEM_SIZE	(0x80)

/* Default FSS mailbox addresses */
#define FSS_SCMI_SH_MEM_0	(0x2483A000)
#define FSS_SCMI_SH_MEM_1	(0x2483B000)
#define FSS_SCMI_SH_MEM_2	(0x2483C000)
#define FSS_SCMI_SH_MEM_3	(0x2483D000)

#define S32N_FSS_SCMI_SH_MEM_SIZE	SZ_4K

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

/**
 * Do not use a populate function, as the properties
 * are not read from the device tree.
 */
struct scmi_ospm_channel_config scmi_ospm_ch_config = {
	.ospm_mem_base = S32N_OSPM_SCMI_MEM,
	.ospm_mem_size = S32N_OSPM_SCMI_MEM_SIZE,
};

struct scmi_sh_mem_config scmi_sh_mem_config = {
	.sh_mem_base = S32N_FSS_SCMI_SH_MEM,
	.sh_mem_size = S32N_FSS_SCMI_SH_MEM_SIZE,
};

struct scmi_fss_channel_config scmi_fss_ch_config = {
	.mb_base = S32N_SCMI_MBX_ADDR,
	.mb_size = S32N_SCMI_MBX_SIZE,
	.mru_ch = FSS_MRU_CH,
};
