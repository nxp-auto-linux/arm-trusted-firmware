/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SCMI_LOGGER_PRIVATE_H
#define SCMI_LOGGER_PRIVATE_H

#include <lib/spinlock.h>

#if defined(IMAGE_BL2)
#define SCMI_LOG_MAX_LEN		64
#elif defined(IMAGE_BL31)
#define SCMI_LOG_MAX_LEN		1024
#else
#define SCMI_LOG_MAX_LEN		1
#endif

#ifndef SCMI_LOG_BUF_LEN
#define SCMI_LOG_BUF_LEN		32
#endif

#define SCMI_LOG_STR_LEN		8

typedef uint64_t entry_state_t;

#define BITS_PER_SLOT			(sizeof(entry_state_t) * 8)
#define SCMI_LOG_SLOTS_NUM		(SCMI_LOG_MAX_LEN / BITS_PER_SLOT +  \
							(!!(SCMI_LOG_MAX_LEN % BITS_PER_SLOT)))

/* General info about a SCMI message */
struct message {
	unsigned int agent_id;
	unsigned int protocol_id;
	unsigned int message_id;
	size_t in_size;
	size_t out_size;
	uint8_t request[SCMI_LOG_BUF_LEN];
	uint8_t response[SCMI_LOG_BUF_LEN];
};

struct scmi_log_entry {
	uint32_t msg_no;
	int32_t idx;
	unsigned int core;
	char type[SCMI_LOG_STR_LEN];
	struct message msg;
};

struct scmi_logger {
	uint32_t msg_count;
	int index;
	spinlock_t lock;

	/* logging interface */
	struct scmi_log_entry* (*get_entry)(unsigned int index);
	void (*log_req_data)(struct scmi_log_entry *entry, uintptr_t md_addr);
	void (*log_rsp_data)(struct scmi_log_entry *entry, uintptr_t md_addr);
	void (*log_notif_data)(struct scmi_log_entry *entry, uintptr_t md_addr);
	void (*log_notif_ack)(struct scmi_log_entry *entry, uintptr_t md_addr);
};

#endif /* SCMI_LOGGER_PRIVATE_H */
