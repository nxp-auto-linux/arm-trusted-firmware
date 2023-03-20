/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SCMI_LOGGER_H
#define SCMI_LOGGER_H

#include "scmi_private.h"

#ifndef SCMI_LOGGER
#define SCMI_LOGGER     0
#endif

void log_scmi_init(void);
void log_scmi_req(mailbox_mem_t *mbx_mem, uintptr_t md_addr);
void log_scmi_rsp(mailbox_mem_t *mbx_mem, uintptr_t md_addr);
void log_scmi_notif(mailbox_mem_t *mbx_mem, uintptr_t md_addr);
void log_scmi_ack(mailbox_mem_t *mbx_mem, uintptr_t md_addr);

#endif /* SCMI_LOGGER_H */

