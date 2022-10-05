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
#include <drivers/scmi.h>
#include <inttypes.h>
#include <dt-bindings/power/s32gen1-scmi-pd.h>

static scmi_channel_t scmi_channels[PLATFORM_CORE_COUNT];
static scmi_channel_plat_info_t s32_scmi_plat_info[PLATFORM_CORE_COUNT];
static void *scmi_handles[PLATFORM_CORE_COUNT];
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
	return S32_SCP_SCMI_MEM + core * S32_SCP_CH_MEM_SIZE;
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

static scmi_channel_t *get_scmi_channel(unsigned int *ch_id)
{
	int core = plat_core_pos_by_mpidr(read_mpidr());
	scmi_channel_t *ch;

	if (core < 0 || core >= (ssize_t)ARRAY_SIZE(scmi_channels)) {
		ERROR("Failed to get SCMI channel for core %d\n",
		      core);
		return NULL;
	}

	ch = &scmi_channels[core];
	if (!ch->is_initialized) {
		scmi_handles[core] = scmi_init(ch);
		if (scmi_handles[core] == NULL) {
			ERROR("Failed to initialize SCMI channel for core %d\n",
			      core);
			return NULL;
		}
	}

	if (ch_id)
		*ch_id = core;

	return ch;
}

static void *get_scmi_handle(void)
{
	unsigned int ch_id;
	scmi_channel_t *ch = get_scmi_channel(&ch_id);

	if (!ch)
		return NULL;

	return scmi_handles[ch_id];
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

void scp_set_core_reset_addr(uintptr_t addr)
{
	int ret;
	void *handle = get_scmi_handle();

	if (!handle) {
		ERROR("%s: Failed to get SCMI handle", __func__);
		panic();
	}

	ret = scmi_ap_core_set_reset_addr(handle, addr, 0x0u);
	if (ret) {
		ERROR("Failed to set core reset address\n");
		panic();
	}
}

static int scp_cpu_set_state(uint32_t core, uint32_t state)
{
	void *handle = get_scmi_handle();
	uint32_t domain_id = S32GEN1_SCMI_PD_A53(core);
	uint32_t pwr_state;
	int ret;

	if (!handle) {
		ERROR("%s: Failed to get SCMI handle", __func__);
		panic();
	}

	pwr_state = S32GEN1_SCMI_PD_SET_LEVEL0_STATE(state);
	pwr_state |= S32GEN1_SCMI_PD_SET_LEVEL1_STATE(S32GEN1_SCMI_PD_ON);
	pwr_state |= S32GEN1_SCMI_PD_SET_MAX_LEVEL_STATE(S32GEN1_SCMI_PD_ON);

	ret = scmi_pwr_state_set(handle, domain_id, pwr_state);

	if (ret != SCMI_E_QUEUED && ret != SCMI_E_SUCCESS) {
		ERROR("Failed to set core%" PRIu32 " power state to '%" PRIu32 "'",
		      core, state);
		return -EINVAL;
	}

	if (ret == SCMI_E_SUCCESS)
		return 0;

	return -EINVAL;
}

int scp_cpu_on(uint32_t core)
{
	return scp_cpu_set_state(core, S32GEN1_SCMI_PD_ON);
}

int scp_cpu_off(uint32_t core)
{
	return scp_cpu_set_state(core, S32GEN1_SCMI_PD_OFF);
}

int scp_get_cpu_state(uint32_t core)
{
	void *handle = get_scmi_handle();
	uint32_t pwr_state = 0u;
	uint32_t domain_id = S32GEN1_SCMI_PD_A53(core);
	int ret;

	if (!handle) {
		ERROR("%s: Failed to get SCMI handle", __func__);
		panic();
	}

	ret = scmi_pwr_state_get(handle, domain_id, &pwr_state);
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to get core%" PRIu32 " power state\n", core);
		return -EINVAL;
	}

	return S32GEN1_SCMI_PD_GET_LEVEL0_STATE(pwr_state);
}

static bool is_proto_allowed(mailbox_mem_t *mbx_mem)
{
	uint32_t proto = SCMI_MSG_GET_PROTO(mbx_mem->msg_header);

	switch (proto) {
	case SCMI_PROTOCOL_ID_BASE:
	case SCMI_PROTOCOL_ID_CLOCK:
	case SCMI_PROTOCOL_ID_RESET_DOMAIN:
		return true;
	}

	return false;
}

int send_scmi_to_scp(uintptr_t scmi_mem)
{
	scmi_channel_plat_info_t *ch_info;
	mailbox_mem_t *mbx_mem;
	unsigned int ch_id;
	scmi_channel_t *ch = get_scmi_channel(&ch_id);

	if (!ch)
		return -EINVAL;

	/* Filter OSPM specific call */
	if (!is_proto_allowed((mailbox_mem_t *)scmi_mem))
		return -EINVAL;

	if (get_packet_size(scmi_mem) > S32_SCP_CH_MEM_SIZE)
		return -EINVAL;

	ch_info = ch->info;
	mbx_mem = (mailbox_mem_t *)(ch_info->scmi_mbx_mem);

	while (!SCMI_IS_CHANNEL_FREE(mbx_mem->status))
		;

	/* Transfer request into SRAM mailbox */
	if (ch_info->scmi_mbx_mem + get_packet_size(scmi_mem) >
	    S32_SCP_SCMI_MEM + S32_SCP_SCMI_MEM_SIZE)
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
