/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32_SVC_H
#define S32_SVC_H

struct scmi_shared_mem {
	uint32_t reserved;
	uint32_t channel_status;
#define SCMI_SHMEM_CHAN_STAT_CHANNEL_ERROR      BIT(1)
#define SCMI_SHMEM_CHAN_STAT_CHANNEL_FREE       BIT(0)
	uint32_t reserved1[2];
	uint32_t flags;
#define SCMI_SHMEM_FLAG_INTR_ENABLED    BIT(0)
	uint32_t length;
	uint32_t msg_header;
	uint8_t msg_payload[];
};

/* Corresponding to msg_payload */
struct response {
	uint32_t status;
	uint32_t data[];
};

#endif /* S32_SVC_H */
