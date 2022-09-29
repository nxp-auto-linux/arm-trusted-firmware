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

DEFINE_BAKERY_LOCK(s32_scmi_lock);

void mscm_ring_doorbell(struct scmi_channel_plat_info *plat_info);

static scmi_channel_plat_info_t s32_scmi_plat_info = {
	.scmi_mbx_mem = SCMI_PAYLOAD_BASE,
	.db_reg_addr = MSCM_BASE_ADDR,
	.db_preserve_mask = 0xfffffffe,
	.db_modify_mask = 0x1,
	.ring_doorbell = &mscm_ring_doorbell,
};

/*
 * The global handle for invoking the SCMI driver APIs after the driver
 * has been initialized.
 */
static void *s32_scmi_handle;

/* The SCMI channel global object */
static scmi_channel_t channel = {
	.info = &s32_scmi_plat_info,
	.lock = &s32_scmi_lock,
};

void mscm_ring_doorbell(struct scmi_channel_plat_info *plat_info)
{
	uintptr_t reg;

	/* Request for M7 Core0, Interrupt 0 */
	assert(!check_uptr_overflow(plat_info->db_reg_addr,
				    MSCM_IRCP4IGR0 - 1));
	reg = plat_info->db_reg_addr + MSCM_IRCP4IGR0;

	mmio_write_32(reg, 1);
}

void scp_scmi_init(void)
{
	s32_scmi_handle = scmi_init(&channel);
	if (s32_scmi_handle == NULL) {
		ERROR("SCMI Initialization failed\n");
		panic();
	}
}

static void copy_scmi_msg(uintptr_t to, uintptr_t from)
{
	size_t copy_len;
	mailbox_mem_t *mbx_mem = (mailbox_mem_t *)from;

	copy_len =  offsetof(mailbox_mem_t, msg_header) + mbx_mem->len;
	memcpy((void *)to, (const void *)from, copy_len);
}

void send_scmi_to_scp(uintptr_t scmi_mem)
{
	scmi_channel_t *ch = &channel;
	scmi_channel_plat_info_t *ch_info = ch->info;
	mailbox_mem_t *mbx_mem = (mailbox_mem_t *)(ch_info->scmi_mbx_mem);

	/* Transfer request into SRAM mailbox */
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
}
