/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_FSS_UTILS_H
#define S32N_FSS_UTILS_H

int scmi_nxp_core_init(void *p);
int scmi_nxp_core_configure_core(void *p, uint32_t core_id, uintptr_t entrypoint);
int scmi_nxp_misc_display_firmware_info(void *p);
int scmi_nxp_misc_display_data_version(void *p);
int scmi_reset_display_plat_reason(void *p);
int scmi_nxp_boot_display_reset_reason(void *p, uint32_t cohort_id);

#endif /* S32N_FSS_UTILS_H */
