/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <common/debug.h>
#include <lib/utils_def.h>
#include <plat/common/platform.h>
#include <scmi-msg/common.h>
#include <string.h>
#include "scmi_logger.h"
#include "scmi_logger_private.h"

enum scmi_msg_type {
	SCMI_REQ,
	SCMI_RSP,
	SCMI_NOTIF,
	SCMI_ACK,
};

/* Platform specific logger init operation */
#pragma weak log_scmi_plat_init
int log_scmi_plat_init(struct scmi_logger *logger)
{
	return 0;
}

static struct scmi_logger logger;
static struct scmi_log_entry *crt_msg[PLATFORM_CORE_COUNT + 1];

/**
 * Each bit in the array represents the state of an SCMI
 * logger entry, as follows:
 *  - 0 = entry is free
 *  - 1 = entry is busy
 * Given a logger entry at index i, the position of the
 * corresponding bit in the array is computed as:
 *  index_of_slot_in_array = i / BITS_PER_SLOT
 *  index_of_bit_in_slot = i % BITS_PER_SLOT
 */
static entry_state_t entry_state[SCMI_LOG_SLOTS_NUM];
/**
 * Used for masking the bits in the last slot that are
 * not actually available (when SCMI_LOG_MAX_LEN is not
 * divisbile by BITS_PER_SLOT).
 */
static entry_state_t slot_mask;

/* Helper function for checking and setting an entry's state */
static inline bool is_logger_empty(void)
{
	return logger.index == -1;
}

static uint32_t get_slot(uint32_t pos)
{
	assert(pos < SCMI_LOG_MAX_LEN);
	return pos / BITS_PER_SLOT;
}

static uint32_t get_bit_in_slot(uint32_t pos)
{
	assert(pos < SCMI_LOG_MAX_LEN);
	return pos % BITS_PER_SLOT;
}

static bool log_scmi_is_entry_free(uint32_t pos)
{
	if (pos >= SCMI_LOG_MAX_LEN)
		return false;

	return !(entry_state[get_slot(pos)] & BIT_64(get_bit_in_slot(pos)));
}

static void log_scmi_mark_entry_busy(uint32_t pos)
{
	assert(pos < SCMI_LOG_MAX_LEN);
	assert(log_scmi_is_entry_free(pos));
	entry_state[get_slot(pos)] |= BIT_64(get_bit_in_slot(pos));
}

static void log_scmi_mark_entry_free(uint32_t pos)
{
	assert(pos < SCMI_LOG_MAX_LEN);
	assert(!log_scmi_is_entry_free(pos));
	entry_state[get_slot(pos)] &= ~BIT_64(get_bit_in_slot(pos));
}

static int get_next_free_pos(int start_pos)
{
	unsigned int slot, bit, next_pos = 0;
	unsigned int max_iterations = SCMI_LOG_SLOTS_NUM;
	entry_state_t state;

	if (is_logger_empty())
		return 0;

	if (start_pos < 0)
		return -1;

	slot = get_slot(start_pos);

	while (max_iterations) {
		bit = get_bit_in_slot(start_pos);

		state = entry_state[slot];
		/**
		 * mask the bits behind current position
		 * and the unavailable ones for the last slot
		 */
		state |= GENMASK_64(bit, 0);
		if (slot == SCMI_LOG_SLOTS_NUM - 1)
			state |= slot_mask;

		next_pos = __builtin_ffsl(~state);
		if (next_pos)
			return slot * BITS_PER_SLOT + next_pos - 1;

		/* if next_pos = 0 -> go to next slot */
		slot = (slot + 1) % SCMI_LOG_SLOTS_NUM;
		start_pos = slot * BITS_PER_SLOT;
		/* check position 0 in slot */
		if (log_scmi_is_entry_free(start_pos))
			return start_pos;

		max_iterations--;
	}

	return -1;
}

static char *type2str(enum scmi_msg_type type)
{
	switch (type) {
	case SCMI_REQ:
	case SCMI_RSP:
		return "cmd";
	case SCMI_NOTIF:
	case SCMI_ACK:
		return "notif";
	default:
		return "unknown";
	}
}

/**
 * Retrieves a free entry in logger and populates its corresponding
 * index and message number. Synchronization should be handled by caller.
 */
static struct scmi_log_entry *get_free_entry(int *pos, uint32_t *msg_no)
{
	struct scmi_log_entry *entry = NULL;

	if (!pos || !msg_no)
		return NULL;

	*pos = get_next_free_pos(logger.index);
	if (*pos >= 0) {
		/* valid position */
		entry = logger.get_entry(*pos);
		log_scmi_mark_entry_busy(*pos);
		*msg_no = logger.msg_count;
		logger.index = *pos;
		logger.msg_count++;
	}

	return entry;
}

static void set_entry_data(struct scmi_log_entry *entry, struct scmi_msg *msg,
		unsigned int core, int idx, uint32_t msg_no, enum scmi_msg_type type)
{
	size_t num_bytes = 0;
	size_t len = 0;

	if (!entry)
		return;

	entry->msg_no = msg_no;
	entry->idx = idx;
	entry->core = core;
	entry->msg.agent_id = msg->agent_id;
	entry->msg.protocol_id = msg->protocol_id;
	entry->msg.message_id = msg->message_id;
	entry->msg.in_size = msg->in_size;
	len = strlcpy(entry->type, type2str(type), SCMI_LOG_STR_LEN);
	if (len >= SCMI_LOG_STR_LEN) {
		WARN("SCMI message type was truncated.\n");
	}

	num_bytes = msg->in_size;
	if (msg->in_size > sizeof(entry->msg.request)) {
		num_bytes = sizeof(entry->msg.request);
		WARN("Incomplete SCMI request logged [msg_no = %d]\n", msg_no);
	}
	memcpy(&entry->msg.request, msg->in, num_bytes);
}

/* Helper functions to save relevant info to log buffer */
static void log_scmi_req_message(struct scmi_msg *msg, uintptr_t md_addr)
{
	struct scmi_log_entry *entry = NULL;
	uint32_t msg_no = 0;
	int pos = 0;
	unsigned int core = plat_my_core_pos();

	if (core >= (ssize_t)ARRAY_SIZE(crt_msg)) {
		ERROR("Failed to get core number %d\n", core);
		return;
	}

	if (!logger.get_entry)
		return;

	spin_lock(&logger.lock);
	entry = get_free_entry(&pos, &msg_no);
	crt_msg[core] = entry;
	spin_unlock(&logger.lock);

	if (entry) {
		set_entry_data(entry, msg, core, pos, msg_no, SCMI_REQ);

		if (logger.log_req_data)
			logger.log_req_data(entry, md_addr);
	} else {
		ERROR("No SCMI Logger entries available.\n");
	}
}

static void log_scmi_rsp_message(struct scmi_msg *msg, uintptr_t md_addr)
{
	struct scmi_log_entry *entry = NULL;
	size_t num_bytes = 0;
	unsigned int core = plat_my_core_pos();

	if (core >= (ssize_t)ARRAY_SIZE(crt_msg)) {
		ERROR("Failed to get core number %d\n", core);
		return;
	}

	entry = crt_msg[core];

	if (entry) {
		entry->msg.out_size = msg->out_size;
		num_bytes = MIN(msg->out_size, sizeof(entry->msg.response));
		memcpy(&entry->msg.response, msg->out, num_bytes);

		if (logger.log_rsp_data)
			logger.log_rsp_data(entry, md_addr);

		spin_lock(&logger.lock);
		log_scmi_mark_entry_free(entry->idx);
		spin_unlock(&logger.lock);
	}
}

static void log_scmi_notif_message(struct scmi_msg *msg, uintptr_t md_addr)
{
	struct scmi_log_entry *entry = NULL;
	uint32_t msg_no = 0;
	int pos = 0;
	unsigned int core = plat_my_core_pos();

	if (core >= (ssize_t)ARRAY_SIZE(crt_msg)) {
		ERROR("Failed to get core number %d\n", core);
		return;
	}

	if (!logger.get_entry)
		return;

	spin_lock(&logger.lock);
	entry = get_free_entry(&pos, &msg_no);
	/* last element for notifications */
	crt_msg[PLATFORM_CORE_COUNT] = entry;
	spin_unlock(&logger.lock);

	if (entry) {
		set_entry_data(entry, msg, core, pos, msg_no, SCMI_NOTIF);

		if (logger.log_notif_data)
			logger.log_notif_data(entry, md_addr);
	} else {
		ERROR("No SCMI Logger entries available.\n");
	}
}

static void log_scmi_notif_ack(struct scmi_msg *msg, uintptr_t md_addr)
{
	struct scmi_log_entry *entry = NULL;

	entry = crt_msg[PLATFORM_CORE_COUNT];

	if (entry) {
		if (logger.log_notif_ack)
			logger.log_notif_ack(entry, md_addr);

		spin_lock(&logger.lock);
		log_scmi_mark_entry_free(entry->idx);
		spin_unlock(&logger.lock);
	}
}

static void log_scmi_raw(mailbox_mem_t *mbx_mem, uintptr_t md_addr, enum scmi_msg_type type)
{
	uint32_t msg_header = mbx_mem->msg_header;

	struct scmi_msg msg = {
		.in = (char *)&mbx_mem->payload[0],
		.in_size = mbx_mem->len - 4,
		.agent_id = 0,
		.protocol_id = SCMI_MSG_GET_PROTO(msg_header),
		.message_id = SCMI_MSG_GET_MSG_ID(msg_header),
		.out = (char *)&mbx_mem->payload[0],
		.out_size = SCMI_MAILBOX_MEM_SIZE - sizeof(*mbx_mem),
	};

	switch (type) {
	case SCMI_REQ:
		log_scmi_req_message(&msg, md_addr);
		break;
	case SCMI_RSP:
		log_scmi_rsp_message(&msg, md_addr);
		break;
	case SCMI_NOTIF:
		log_scmi_notif_message(&msg, md_addr);
		break;
	case SCMI_ACK:
		log_scmi_notif_ack(&msg, md_addr);
		break;
	default:
		break;
	}
}

void log_scmi_init(void)
{
	unsigned int extra_bits = SCMI_LOG_MAX_LEN % BITS_PER_SLOT;

	if (extra_bits)
		slot_mask = ~GENMASK(extra_bits - 1, 0);

	logger.index = -1;

	if (log_scmi_plat_init(&logger))
		ERROR("Could not init SCMI logger.\n");
}

void log_scmi_req(mailbox_mem_t *mbx_mem, uintptr_t md_addr)
{
	log_scmi_raw(mbx_mem, md_addr, SCMI_REQ);
}

void log_scmi_rsp(mailbox_mem_t *mbx_mem, uintptr_t md_addr)
{
	log_scmi_raw(mbx_mem, md_addr, SCMI_RSP);
}

void log_scmi_notif(mailbox_mem_t *mbx_mem, uintptr_t md_addr)
{
	log_scmi_raw(mbx_mem, md_addr, SCMI_NOTIF);
}

void log_scmi_ack(mailbox_mem_t *mbx_mem, uintptr_t md_addr)
{
	log_scmi_raw(mbx_mem, md_addr, SCMI_ACK);
}
