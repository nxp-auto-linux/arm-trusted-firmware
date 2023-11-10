/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32_SCP_SCMI_H
#define S32_SCP_SCMI_H

#include <stdint.h>

#define SCMI_PROTOCOL_ID_PINCTRL	(0x80u)
#define SCMI_PROTOCOL_ID_GPIO		(0x81u)
#define SCMI_PROTOCOL_ID_NVMEM		(0x82u)

#define S32_SCP_BUF_SIZE			(128)

typedef int (*scmi_msg_callback_t)(void *payload);

int scp_scmi_dt_init(bool init_rx);
void scp_scmi_init(bool request_irq);
int scp_get_rx_plat_irq(void);
int send_scmi_to_scp(uintptr_t scmi_mem, size_t scmi_mem_size);
void scp_set_core_reset_addr(uintptr_t addr);
int scp_get_cpu_state(uint32_t core);
int scp_cpu_on(uint32_t core);
int scp_cpu_off(uint32_t core);
void scp_suspend_platform(void);
void __dead2 scp_shutdown_platform(void);
void __dead2 scp_reset_platform(void);

int register_scmi_internal_msg_handler(uint32_t protocol, uint32_t msg_id,
				       scmi_msg_callback_t callback);
#endif
