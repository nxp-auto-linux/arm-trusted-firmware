/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_FSS_SCMI_H
#define S32N_FSS_SCMI_H

#include <stdbool.h>
#include <stdint.h>

#define SCMI_PROTOCOL_ID_NXP_BOOT   U(0x81)
#define SCMI_PROTOCOL_ID_NXP_CORE   U(0x83)
#define SCMI_PROTOCOL_ID_NXP_MISC	U(0x86)

void fss_scmi_init_a2p(void);
void fss_scmi_init_p2a(void);
int fss_scmi_send_sync_cmd(uintptr_t scmi_mem, size_t scmi_mem_size);
int fss_handshake_init(bool timeout);
int fss_cpu_on(uint32_t pos);
void __dead2 fss_cpu_off(uint32_t pos);
void __dead2 fss_system_suspend(void);
void __dead2 fss_cohort_reset(void);
void __dead2 fss_cohort_shutdown(void);
int fss_get_node_hw_state(uint32_t core, unsigned int level, unsigned int *state);
int fss_get_clock_rate(uint32_t clk_id, unsigned long *rate);
int fss_scmi_nxp_core_validate(void);
int fss_set_cores_reset_addr(uintptr_t core_addr);
int fss_get_misc_info(void);
int fss_get_gic_mode(uint8_t *gic_mode);
int fss_display_reset_plat_reason(void);
int fss_display_reset_cohort_reason(uint32_t cohort_id);

uintptr_t get_db_rx_base(void);
size_t get_db_rx_size(void);
uint32_t get_aps_mru_irq(void);
uint8_t get_aps_mru_chan(void);
uint8_t get_aps_mru_mbox(void);

#endif /* S32N_FSS_SCMI_H */
