/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <libc/assert.h>
#include <common/debug.h>
#include <drivers/arm/css/scmi.h>
#include <arm/css/scmi/scmi_private.h>
#include <lib/mmio.h>
#include <platform.h>
#include <libc/errno.h>

static scmi_channel_t scmi_channels[PLATFORM_CORE_COUNT];
static scmi_channel_plat_info_t s32_scmi_plat_info[PLATFORM_CORE_COUNT];
DEFINE_BAKERY_LOCK(s32_scmi_locks[PLATFORM_CORE_COUNT]);

void mscm_ring_doorbell(struct scmi_channel_plat_info *plat_info)
{
	uintptr_t reg;

	/* Request for M7 Core0, Interrupt 0 */
	assert(!check_uptr_overflow(plat_info->db_reg_addr,
				    MSCM_IRCP4IGR0 - 1));
	reg = plat_info->db_reg_addr + MSCM_IRCP4IGR0;

	mmio_write_32(reg, 1);
}

static uintptr_t get_mb_addr(uint32_t core)
{
	return SCMI_PAYLOAD_BASE + core * SCMI_PAYLOAD_SIZE;
}

void scp_scmi_init(void)
{
	size_t i;

	assert(ARRAY_SIZE(scmi_channels) == ARRAY_SIZE(s32_scmi_locks));
	assert(ARRAY_SIZE(scmi_channels) == ARRAY_SIZE(s32_scmi_plat_info));

	for (i = 0u; i < ARRAY_SIZE(scmi_channels); i++) {
		s32_scmi_plat_info[i] = (scmi_channel_plat_info_t) {
			.scmi_mbx_mem = get_mb_addr(i),
			.db_reg_addr = MSCM_BASE_ADDR,
			.db_preserve_mask = 0xfffffffe,
			.db_modify_mask = 0x1,
			.ring_doorbell = &mscm_ring_doorbell,
		};

		scmi_channels[i] = (scmi_channel_t) {
			.info = &s32_scmi_plat_info[i],
			.lock = &s32_scmi_locks[i],
		};
	}
}

static scmi_channel_t *get_scmi_channel(int *ch_id)
{
	int core = plat_core_pos_by_mpidr(read_mpidr());

	if (core >= ARRAY_SIZE(scmi_channels)) {
		ERROR("Failed to get SCMI channel for core %d\n",
		      core);
		return NULL;
	}

	*ch_id = core;
	return &scmi_channels[core];
}

static size_t get_packet_size(uintptr_t scmi_packet)
{
	mailbox_mem_t *mbx_mem = (mailbox_mem_t *)scmi_packet;

	return offsetof(mailbox_mem_t, msg_header) + mbx_mem->len;
}

static void copy_scmi_msg(uintptr_t to, uintptr_t from)
{
	size_t copy_len;

	copy_len = get_packet_size(from);
	memcpy((void *)to, (const void *)from, copy_len);
}

int send_scmi_to_scp(uintptr_t scmi_mem)
{
	scmi_channel_plat_info_t *ch_info;
	mailbox_mem_t *mbx_mem;
	void *s32_scmi_handle;
	int ch_id;
	scmi_channel_t *ch = get_scmi_channel(&ch_id);

	if (!ch)
		return -EINVAL;

	if (get_packet_size(scmi_mem) > SCMI_PAYLOAD_SIZE)
		return -EINVAL;

	if (!ch->is_initialized) {
		s32_scmi_handle = scmi_init(ch);
		if (s32_scmi_handle == NULL) {
			ERROR("Failed to initialize SCMI channel for core %d\n",
			      ch_id);
			return -EINVAL;
		}
	}

	ch_info = ch->info;
	mbx_mem = (mailbox_mem_t *)(ch_info->scmi_mbx_mem);

	while (!SCMI_IS_CHANNEL_FREE(mbx_mem->status))
		;

	/* Transfer request into SRAM mailbox */
	if (ch_info->scmi_mbx_mem + get_packet_size(scmi_mem) >
	    SCMI_PAYLOAD_BASE + SCMI_PAYLOAD_MAX_SIZE)
		return -EINVAL;

	copy_scmi_msg((uintptr_t)mbx_mem, scmi_mem);

	SCMI_MARK_CHANNEL_FREE(mbx_mem->status);

	/*
	 * All commands must complete with a poll, not
	 * an interrupt, even if the agent requested this.
	 */
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;

	validate_scmi_channel(ch);

	scmi_get_channel(ch);

	scmi_send_sync_command(ch);

	scmi_put_channel(ch);

	/* Copy the result to agent's space */
	copy_scmi_msg(scmi_mem, (uintptr_t)mbx_mem);

	return 0;
}
