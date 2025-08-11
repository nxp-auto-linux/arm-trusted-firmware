/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_FSS_SCMI_H
#define S32N_FSS_SCMI_H

#include <stdint.h>

#define SCMI_PROTOCOL_ID_NXP_BOOT   U(0x81)
#define SCMI_PROTOCOL_ID_NXP_CORE   U(0x83)
#define SCMI_PROTOCOL_ID_NXP_MISC	U(0x86)

void fss_scmi_init(void);
int fss_scmi_send_sync_cmd(uintptr_t scmi_mem, size_t scmi_mem_size);
int fss_cpu_on(uint32_t pos);
int fss_cpu_off(uint32_t pos);
void __dead2 fss_system_suspend(void);
int fss_scmi_nxp_core_validate(void);
int fss_set_cores_reset_addr(uintptr_t core_addr);
int fss_get_misc_info(void);
int fss_get_gic_mode(uint8_t *gic_mode);
int fss_display_reset_plat_reason(void);
int fss_display_reset_cohort_reason(uint32_t cohort_id);

#endif /* S32N_FSS_SCMI_H */
