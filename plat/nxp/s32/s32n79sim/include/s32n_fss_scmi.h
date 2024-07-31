/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_FSS_SCMI_H
#define S32N_FSS_SCMI_H

#include <stdint.h>

void fss_scmi_init(void);
int fss_scmi_send_sync_cmd(uintptr_t scmi_mem, size_t scmi_mem_size);
int fss_cpu_on(uint32_t pos);
int fss_cpu_off(uint32_t pos);
void fss_scmi_ap_core_init(void);

#endif /* S32N_FSS_SCMI_H */
