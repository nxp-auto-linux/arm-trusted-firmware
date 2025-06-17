/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_FSS_UTILS_H
#define S32N_FSS_UTILS_H

int scmi_nxp_core_configure_core(void *p, uint32_t core_id, uintptr_t entrypoint);
int scmi_nxp_misc_display_firmware_info(void *p);
int scmi_nxp_misc_display_data_version(void *p);

#endif /* S32N_FSS_UTILS_H */
