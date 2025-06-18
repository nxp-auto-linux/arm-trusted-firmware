// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2025 NXP
 */

#ifndef S32N_SCMI_NXP_MISC_H
#define S32N_SCMI_NXP_MISC_H

#include <lib/utils_def.h>

#define FSS_FIRMWARE_MAJOR_VER_MASK		GENMASK_32(31, 24)
#define FSS_FIRMWARE_MAJOR_VER_SHIFT	(24)
#define FSS_FIRMWARE_MINOR_VER_MASK		GENMASK_32(23, 16)
#define FSS_FIRMWARE_MINOR_VER_SHIFT	(16)
#define FSS_FIRMWARE_PATCH_VER_MASK		GENMASK_32(15, 8)
#define FSS_FIRMWARE_PATCH_VER_SHIFT	(8)
#define FSS_FIRMWARE_CODE_DROP_VER_MASK	GENMASK_32(7, 0)

/*
 * Identifiers of the NXP Misc Protocol commands
 */
enum scmi_nxp_misc_command_id {
	SCMI_NXP_GET_FIRMWAREINFO = 0x3,
	SCMI_NXP_GET_DATA_VERSION = 0x5,
};

/*
 * NXP Misc Get Firmware Info
 */
struct scmi_nxp_misc_get_firmware_info_p2a {
	int32_t status;
	uint32_t hotfix_version;
	uint32_t version;
	uint8_t build_date[12];
	uint8_t build_time[12];
	uint32_t firmware_type;
	uint32_t derivative;
	uint32_t hw_support;
	uint32_t external_device_support;
} __packed;

/*
 * NXP Misc Get Data Version
 */
struct scmi_nxp_misc_get_data_version_p2a {
	int32_t status;
	uint32_t boot_data_version;
	uint32_t clock_data_version;
	uint32_t dsc_irq_data_version;
	uint32_t eim_data_version;
	uint32_t erm_data_version;
	uint32_t fccu_data_version;
	uint32_t mem_data_version;
	uint32_t pin_data_version;
	uint32_t pwr_data_version;
	uint32_t rst_data_version;
	uint32_t secman_data_version;
	uint32_t sns_data_version;
	uint32_t swpu_data_version;
} __packed;

#endif	/* S32N_SCMI_NXP_MISC_H */
