/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <errno.h>
#include <platform.h>
#include <common/bl_common.h>
#include <drivers/scmi.h>
#include <drivers/arm/css/scmi.h>
#include <drivers/nxp/s32n/s32n_scmi_extensions.h>
#include <dt-bindings/nxp/s32n79-scmi-nxp-core.h>
#include <dt-bindings/power/s32n79-scmi-pd.h>
#include <inttypes.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <scmi-msg/base.h>
#include <s32n_fss_mru.h>
#include <s32_bl_common.h>
#include <s32_scmi.h>
#include <s32n_aps_mru.h>
#include <s32n_bl_common.h>
#include <s32n_fss_scmi.h>
#include <s32n_fss_utils.h>
#include <s32n_fconf_scmi_getter.h>
#include <s32n_fconf_gic_getter.h>

static const uint32_t core_power_domain_ids[] = {
	S32N79_PD_APS0_CORE0,
	S32N79_PD_APS0_CORE1,
	S32N79_PD_APS1_CORE0,
	S32N79_PD_APS1_CORE1,
	S32N79_PD_APS2_CORE0,
	S32N79_PD_APS2_CORE1,
	S32N79_PD_APS3_CORE0,
	S32N79_PD_APS3_CORE1,
};

typedef enum {
	CPU_ON,
	CPU_OFF,
} cpu_power_state_t;

/* NXP Core Protocol IDs */
static const uint32_t nxp_core_ids[] = {
	S32N79_CORE_APS0_CORE0,
	S32N79_CORE_APS0_CORE1,
	S32N79_CORE_APS1_CORE0,
	S32N79_CORE_APS1_CORE1,
	S32N79_CORE_APS2_CORE0,
	S32N79_CORE_APS2_CORE1,
	S32N79_CORE_APS3_CORE0,
	S32N79_CORE_APS3_CORE1,
};

#define S32N_SCMI_TIMEOUT_US (10 * 1000000U) /* 10s */

/* Magic value used to ring the FSS MRU doorbell */
#define RING_DB_VAL			0x49504346U

/* Magic number to indicate the messaging channel is initialized */
#define SHM_STATE_READY		0x3252455646435049ULL

static scmi_channel_t tx_scmi_channel;
static scmi_channel_t rx_scmi_channel;
static scmi_channel_plat_info_t s32n_scmi_plat_info_tx;
static scmi_channel_plat_info_t s32n_scmi_plat_info_rx;
static void *rx_scmi_handle, *tx_scmi_handle;
static spinlock_t s32n_scmi_lock_tx, s32n_scmi_lock_rx;

static uintptr_t get_local_handshake_addr(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_sh_mem_config, local_handshake_addr);
}

static uintptr_t get_remote_handshake_addr(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_sh_mem_config, remote_handshake_addr);
}

static unsigned int get_core_global_id(uint32_t core)
{
	unsigned int gic_instance = FCONF_GET_PROPERTY(hw_config, gic_config, instance);

	return gic_instance * PLATFORM_MAX_CPUS_PER_CLUSTER + core;
}

static unsigned int get_plat_core_count(void)
{
	return PLATFORM_CLUSTER_COUNT * PLATFORM_MAX_CPUS_PER_CLUSTER;
}

static uint32_t get_core_power_domain_id(uint32_t core)
{
	unsigned int pos = get_core_global_id(core);

	assert(pos < ARRAY_SIZE(core_power_domain_ids));
	return core_power_domain_ids[pos];
}

static uint32_t get_core_nxp_core_id(uint32_t core)
{
	unsigned int pos = get_core_global_id(core);

	assert(pos < ARRAY_SIZE(nxp_core_ids));
	return nxp_core_ids[pos];
}

static bool is_primary_core(uint32_t core)
{
	return core == S32N_PLAT_PRIMARY_CPU;
}

/** Check FSS ready state */
static bool is_fss_ready(void *arg)
{
	uintptr_t remote_addr = get_remote_handshake_addr();

	return mmio_read_64(remote_addr) == SHM_STATE_READY;
}

int fss_handshake_init(bool timeout)
{
	uintptr_t local_addr = get_local_handshake_addr();
	int ret;

	if (timeout)
		ret = wait_for_condition_timeout(is_fss_ready,
			NULL, S32N_SCMI_TIMEOUT_US);
	else
		ret = wait_for_condition(is_fss_ready, NULL);

	if (ret == 0)
		mmio_write_64(local_addr, SHM_STATE_READY);

	return ret;
}

static scmi_channel_t *scmi_chan_init(scmi_channel_t *ch)
{
	int ret;

	assert(ch && ch->info);
	assert(ch->info->ring_doorbell != NULL);
	assert(ch->lock);
	assert(ch->info->db_reg_addr);

	ch->is_initialized = 0;

	if (!ch->info->scmi_mbx_mem)
		return NULL;

	ret = fss_handshake_init(true);
	if (ret) {
		ERROR("FSS SCMI handshake failed\n");
		return NULL;
	}

	ch->is_initialized = 1;
	return ch;
}

static scmi_channel_t *get_scmi_channel(scmi_dir_t dir)
{
	scmi_channel_t *channel = &tx_scmi_channel;

	/*
	 * Only A2P channel (tx_scmi_channel) needs to be initialized by the
	 * agent (TF-A). P2A channel (rx_scmi_channel) should be initialized
	 * by the SCMI Platform.
	 */
	if (dir == SCMI_RX)
		return &rx_scmi_channel;

	if (!channel->is_initialized) {
		tx_scmi_handle = scmi_chan_init(channel);
		if (!tx_scmi_handle) {
			ERROR("Could not initialize SCMI TX (A2P) channel.\n");
			panic();
		}
	}

	return channel;
}

static void *get_scmi_handle(scmi_dir_t dir)
{
	scmi_channel_t *ch = get_scmi_channel(dir);

	if (!ch)
		return NULL;

	if (dir == SCMI_RX)
		return rx_scmi_handle;

	return tx_scmi_handle;
}

static uintptr_t get_scmi_tx_mb_addr(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_fss_ch_tx_config, mb_base);
}

static size_t get_scmi_tx_mb_size(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_fss_ch_tx_config, mb_size);
}

static uintptr_t get_scmi_rx_mb_addr(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_fss_ch_rx_config, mb_base);
}

static size_t get_scmi_rx_mb_size(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_fss_ch_rx_config, mb_size);
}

static uintptr_t get_db_tx_reg_addr(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_doorbell_tx_config, db_addr);
}

static uintptr_t get_db_rx_reg_addr(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_doorbell_rx_config, db_addr);
}

static uintptr_t get_db_tx_base(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_doorbell_tx_config, base);
}

uintptr_t get_db_rx_base(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_doorbell_rx_config, base);
}

static size_t get_db_tx_size(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_doorbell_tx_config, size);
}

size_t get_db_rx_size(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_doorbell_rx_config, size);
}

uint32_t get_aps_mru_irq(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_aps_mru_config, irq);
}

uint8_t get_aps_mru_chan(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_aps_mru_config, chan);
}

uint8_t get_aps_mru_mbox(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_aps_mru_config, mbox);
}

static void fss_ring_doorbell(struct scmi_channel_plat_info *plat_info)
{
	mmio_write_32(plat_info->db_reg_addr, RING_DB_VAL);
}

static bool is_scmi_channel_free(void *mailbox)
{
	mailbox_mem_t *mbx_mem = (mailbox_mem_t *)mailbox;

	return SCMI_IS_CHANNEL_FREE(mbx_mem->status);
}

static int fss_scmi_receive(uintptr_t scmi_mem, size_t scmi_mem_size, scmi_channel_t *ch)
{
	scmi_channel_plat_info_t *info = ch->info;
	int ret;

	if (!info->scmi_mbx_mem)
		return SCMI_GENERIC_ERROR;

	/* Wait for channel to be free */
	ret = wait_for_condition_timeout(is_scmi_channel_free,
			(void *)info->scmi_mbx_mem, S32N_SCMI_TIMEOUT_US);
	if (ret)
		return SCMI_COMMS_ERROR;

	/*
	 * Ensure that any read from the SCMI payload area is done after reading
	 * the mailbox status.
	 */
	dmbld();

	/* Copy the response back to the OSPM mailbox */
	ret = copy_scmi_msg(scmi_mem, info->scmi_mbx_mem, scmi_mem_size);
	if (ret)
		return SCMI_OUT_OF_RANGE;

	return SCMI_SUCCESS;
}

static int fss_scmi_send(uintptr_t scmi_mem, scmi_channel_t *ch)
{
	scmi_channel_plat_info_t *info = ch->info;
	int ret;

	if (!info->scmi_mbx_mem)
		return SCMI_GENERIC_ERROR;

	/* Wait for channel to be free */
	ret = wait_for_condition_timeout(is_scmi_channel_free,
			(void *)info->scmi_mbx_mem, S32N_SCMI_TIMEOUT_US);
	if (ret)
		return SCMI_COMMS_ERROR;

	/*
	 * Ensure that any read from the SCMI payload area is done after reading
	 * the mailbox status.
	 */
	dmbld();

	/* Copy the request to the mailbox */
	ret = copy_scmi_msg(info->scmi_mbx_mem, scmi_mem, info->scmi_mbx_size);
	if (ret)
		return SCMI_OUT_OF_RANGE;

	/*
	 * Mark the channel busy if needed. As the entire SCMI message was copied,
	 * the busy flag should be already set.
	 */
	if (is_scmi_channel_free((void *)info->scmi_mbx_mem))
		SCMI_MARK_CHANNEL_BUSY(((mailbox_mem_t *)info->scmi_mbx_mem)->status);

	/*
	 * Ensure that writing to the doorbell register happens only after any other
	 * writes to the SCMI mailbox area.
	 */
	dmbst();

	/* Ring doorbell */
	ch->info->ring_doorbell(ch->info);

	/*
	 * Ensure that the write to the doorbell register is ordered prior to
	 * reading from the SCMI mailbox.
	 */
	dmbsy();

	return SCMI_SUCCESS;
}

int fss_scmi_send_sync_cmd(uintptr_t scmi_mem, size_t scmi_mem_size)
{
	scmi_channel_t *ch = get_scmi_channel(SCMI_TX);
	int ret;

	if (!ch || !ch->info || !ch->is_initialized)
		return SCMI_GENERIC_ERROR;

	spin_lock(ch->lock);
	ret = fss_scmi_send(scmi_mem, ch);
	if (ret) {
		spin_unlock(ch->lock);
		return ret;
	}

	ret = fss_scmi_receive(scmi_mem, scmi_mem_size, ch);
	spin_unlock(ch->lock);

	return ret;
}

static void mmap_add_scmi_sh_mem(uintptr_t addr, size_t size)
{
	uintptr_t base;
	int ret;

	if (!size || round_up_overflow(size, PAGE_SIZE, &size)) {
		ERROR("Invalid SCMI shared memory size: 0x%lx\n", size);
		panic();
	}

	base = page_align(addr, DOWN);

	ret = mmap_add_dynamic_region(base, base, size,
			MT_NON_CACHEABLE | MT_RW | MT_SECURE);
	if (ret < 0) {
		ERROR("Error %d mapping SCMI shared mem 0x%lx\n", ret, base);
		panic();
	}
}

static void process_base_notif(mailbox_mem_t *mb)
{
	struct base_error_event *err_event;
	uint32_t i, err_count;

	err_event = (struct base_error_event *)mb->payload;
	NOTICE("Received Base Error Event for agent: %d\n",
	       err_event->agent_id);

	if (err_event->error_status & ERROR_STATUS_FATAL_MASK) {
		ERROR("Received error is fatal and platform cannot continue\n");
		panic();
	}

	err_count = err_event->error_status & ERROR_STATUS_COUNT_MASK;
	if (err_count > SCMI_MAX_ERROR_TUPLES)
		err_count = SCMI_MAX_ERROR_TUPLES;

	for (i = 0; i < err_count; i++)
		NOTICE("Received Command: %d with status: %d\n",
		       err_event->err_event_tuple[i].msg_header,
		       err_event->err_event_tuple[i].status);
}

static void process_sys_power_state_notif(mailbox_mem_t *mb)
{
	struct sys_power_event *sys_pwr_event;

	sys_pwr_event = (struct sys_power_event *)mb->payload;
	NOTICE("Received System Power State Notification for agent: %d\n",
	       sys_pwr_event->agent_id);

	if (sys_pwr_event->flags & SYS_PWR_FORCEFUL_REQ_MASK)
		NOTICE("Graceful power state transition has been requested\n");
	else
		NOTICE("Forceful power state transition has been requested\n");

	switch (sys_pwr_event->system_state) {
	case S32_SYS_PWR_SHUTDOWN_VAL:
		NOTICE("System will transition to Shutdown Power state\n");
		/* Timeout is valid if:
		 *   - System is shutdown
		 *   - Graceful power state transition has been requested
		 */
		if (sys_pwr_event->flags & SYS_PWR_FORCEFUL_REQ_MASK)
			NOTICE("System shutdown will begin in: %d msec\n",
			       sys_pwr_event->timeout);
		break;
	case S32_SYS_PWR_STOP_VAL:
		NOTICE("System will transition to Stop Power state\n");
		break;
	case S32_SYS_PWR_SUSPEND_VAL:
		NOTICE("System will transition to Suspend Power state\n");
		break;
	case S32_SYS_PWR_SUSPEND_TO_RAM_VAL:
		NOTICE("System will transition to Suspend-to-RAM Power state\n");
		break;
	case S32_SYS_PWR_WARM_RESET_VAL:
		NOTICE("System will transition to Warm Reset Power state\n");
		break;
	default:
		ERROR("Invalid System Power state transition\n");
		break;
	}
}

uint64_t mru_interrupt_handler(uint32_t id,
			       uint32_t flags, void *handle, void *cookie)
{
	uintptr_t mb_addr = get_scmi_rx_mb_addr();
	mailbox_mem_t *mb = (mailbox_mem_t *)mb_addr;
	size_t pkt_size = get_packet_size(mb_addr);
	uintptr_t mru_addr = get_db_rx_base();
	uint32_t proto;

	if (SCMI_IS_CHANNEL_FREE(mb->status)) {
		VERBOSE("Spurious MRU irq - channel is free\n");
		return 0;
	}

	if (pkt_size > get_scmi_rx_mb_size()) {
		ERROR("SCMI packet too large: %zu > %zu\n",
		      pkt_size, get_scmi_rx_mb_size());
		goto free_scmi_rx_channel;
	}

	if (mru_irq_read_data(mru_addr, get_aps_mru_chan(),
			      get_aps_mru_mbox()))
		goto free_scmi_rx_channel;

	proto = SCMI_MSG_GET_PROTO(mb->msg_header);
	switch (proto) {
	case SCMI_PROTOCOL_ID_BASE:
		process_base_notif(mb);
		break;
	case SCMI_PROTOCOL_ID_POWER_DOMAIN:
		/* Currently unsupported by FSS FW */
		break;
	case SCMI_PROTOCOL_ID_SYS_POWER:
		process_sys_power_state_notif(mb);
		break;
	case SCMI_PROTOCOL_ID_PERF:
		/* To be implemented */
		break;
	case SCMI_PROTOCOL_ID_CLOCK:
		/* Currently unsupported by FSS FW */
		break;
	case SCMI_PROTOCOL_ID_SENSOR:
		/* To be implemented */
		break;
	case SCMI_PROTOCOL_ID_RESET_DOMAIN:
		/* Currently unsupported by FSS FW */
		break;
	default:
		break;
	}

free_scmi_rx_channel:
	/* Mark the channel as free */
	SCMI_MARK_CHANNEL_FREE(mb->status);
	return 0;
}

static int fss_subscribe_base_error_notify(void)
{
	void *handle = get_scmi_handle(SCMI_TX);

	if (!handle)
		return -EINVAL;

	return scmi_subscribe_base_error_notify(handle, SCMI_NOTIFY_ENABLE);
}

static int fss_subscribe_sys_power_state_notify(void)
{
	void *handle = get_scmi_handle(SCMI_TX);

	if (!handle)
		return -EINVAL;

	return scmi_sys_pwr_state_notify(handle, SCMI_NOTIFY_ENABLE);
}

void fss_scmi_init_a2p(void)
{
	int ret;

	s32n_scmi_plat_info_tx = (scmi_channel_plat_info_t) {
		.scmi_mbx_mem = get_scmi_tx_mb_addr(),
		.scmi_mbx_size = get_scmi_tx_mb_size(),
		.db_reg_addr = get_db_tx_reg_addr(),
		.ring_doorbell = &fss_ring_doorbell,
	};

	tx_scmi_channel = (scmi_channel_t) {
		.info = &s32n_scmi_plat_info_tx,
		.lock = &s32n_scmi_lock_tx,
	};

	/* Assumption: the SCMI mailbox, local and remote handshake addresses are
	 * part of the same 4KB shared memory area. Custom configurations might require
	 * separate memory mapping if the addresses span across different pages.
	 */
	mmap_add_scmi_sh_mem(get_scmi_tx_mb_addr(), get_scmi_tx_mb_size());

	ret = mmap_add_dynamic_region(get_db_tx_base(), get_db_tx_base(),
				      MMU_ROUND_UP_TO_PAGE(get_db_tx_size()),
				      MT_DEVICE | MT_RW | MT_SECURE);
	if (ret < 0) {
		ERROR("Error mapping FSS TX MRU region %d\n", ret);
		panic();
	}
}

void fss_scmi_init_p2a(void)
{
	uintptr_t mbox_tx_addr = get_scmi_tx_mb_addr();
	uintptr_t mbox_rx_addr = get_scmi_rx_mb_addr();
	uintptr_t mbox_range_diff;
	int ret;

	s32n_scmi_plat_info_rx = (scmi_channel_plat_info_t) {
		.scmi_mbx_mem = get_scmi_rx_mb_addr(),
		.scmi_mbx_size = get_scmi_rx_mb_size(),
		.db_reg_addr = get_db_rx_reg_addr(),
		.ring_doorbell = NULL,
	};

	rx_scmi_channel = (scmi_channel_t) {
		.info = &s32n_scmi_plat_info_rx,
		.lock = &s32n_scmi_lock_rx,
	};

	/* Handle the case when SCMI TX and RX Mailboxes are not part of the
	 * same 4KB memory page.
	 */
	if (mbox_rx_addr > mbox_tx_addr)
		mbox_range_diff = mbox_rx_addr - mbox_tx_addr;
	else
		mbox_range_diff = mbox_tx_addr - mbox_rx_addr;

	if (mbox_range_diff > PAGE_SIZE)
		mmap_add_scmi_sh_mem(mbox_rx_addr, get_scmi_rx_mb_size());

	ret = mmap_add_dynamic_region(get_db_rx_base(), get_db_rx_base(),
				      MMU_ROUND_UP_TO_PAGE(get_db_rx_size()),
				      MT_DEVICE | MT_RW | MT_SECURE);
	if (ret < 0) {
		ERROR("Error mapping FSS RX MRU region %d\n", ret);
		panic();
	}

	/* Subscribe to Base Error Notif */
	ret = fss_subscribe_base_error_notify();
	if (ret)
		ERROR("Error subscribing to SCMI Base Notify Errors\n");

	/* Subscribe to System Power State Notif */
	ret = fss_subscribe_sys_power_state_notify();
	if (ret)
		ERROR("Error subscribing to SCMI Sys Pwr State Events\n");
}

static int fss_cpu_set_state(uint32_t core, cpu_power_state_t state)
{
	void *handle = get_scmi_handle(SCMI_TX);
	uint32_t domain_id = get_core_power_domain_id(core);
	uint32_t pwr_state = 0;
	int ret;

	if (!handle) {
		ERROR("%s: Failed to get SCMI handle", __func__);
		return -ENODEV;
	}

	switch (state) {
	case CPU_ON:
		pwr_state = S32N_SCMI_PD_SET_LEVEL0_STATE(S32N_SCMI_PD_LEVEL_ON);
		pwr_state |= S32N_SCMI_PD_SET_LEVEL1_STATE(S32N_SCMI_PD_LEVEL_ON);
		break;
	case CPU_OFF:
		pwr_state = S32N_SCMI_PD_SET_LEVEL0_STATE(S32N_SCMI_PD_LEVEL_OFF);
		pwr_state |= S32N_SCMI_PD_SET_LEVEL1_STATE(S32N_SCMI_PD_LEVEL_OFF);
		pwr_state |= S32N_SCMI_PD_SET_STATE_TYPE(S32N_SCMI_PD_STATE_PWR_DWN);
		break;
	default:
		ERROR("Invalid power state requested.\n");
		return -EINVAL;
	}

	ret = scmi_pwr_state_set(handle, domain_id, pwr_state);
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to set core%" PRIu32 " power state to '0x%" PRIx32 "'\n",
		      core, pwr_state);
		return -EINVAL;
	}

	return 0;
}

int fss_get_node_hw_state(uint32_t core, unsigned int level, unsigned int *state)
{
	uint32_t domain_id = get_core_power_domain_id(core);
	void *handle = get_scmi_handle(SCMI_TX);
	uint32_t pwr_state = 0u;
	int ret;

	if (!handle) {
		ERROR("%s: Failed to get SCMI handle", __func__);
		return -ENODEV;
	}

	/* returns the composite power state */
	ret = scmi_pwr_state_get(handle, domain_id, &pwr_state);
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to get domain %" PRIu32 " power state\n", domain_id);
		return -EINVAL;
	}

	switch (level) {
	case S32N_PWR_LVL0:
		/* CPU */
		*state = S32N_SCMI_PD_GET_LEVEL0_STATE(pwr_state);
		break;
	case S32N_PWR_LVL1:
		/* Cluster */
		*state = S32N_SCMI_PD_GET_LEVEL1_STATE(pwr_state);
		break;
	default:
		ERROR("Invalid power level requested.\n");
		return -EINVAL;
	}

	return ret;
}

static int fss_set_sys_pwr_state(uint32_t state)
{
	void *handle = get_scmi_handle(SCMI_TX);
	int ret;

	if (!handle) {
		ERROR("%s: Failed to get SCMI handle", __func__);
		return -ENODEV;
	}

	ret = scmi_sys_pwr_state_set(handle, SCMI_SYS_PWR_GRACEFUL_REQ, state);
	if (ret != SCMI_E_SUCCESS)
		ERROR("Failed to change power state of the system.\n");

	return ret;
}

/*
 * The generic scmi_sys_pwr_state_set() busy-waits for FSS to mark the A2P
 * channel free after the request. For SHUTDOWN/COLD_RESET the current FSS
 * firmware (1.24) does not release the channel, so that wait would hang the
 * requesting core forever. Frame the SYSTEM_POWER_STATE_SET message by hand,
 * ring the FSS doorbell and return immediately without waiting for the
 * response, letting the caller proceed to park the core. Drop this once FSS
 * reliably frees the channel.
 */
static int fss_set_sys_pwr_state_nowait(uint32_t state)
{
	scmi_channel_t *ch = get_scmi_channel(SCMI_TX);
	mailbox_mem_t *mbx_mem;

	if (!ch || !ch->info || !ch->is_initialized) {
		ERROR("%s: SCMI TX channel not available\n", __func__);
		return SCMI_GENERIC_ERROR;
	}

	spin_lock(ch->lock);

	mbx_mem = (mailbox_mem_t *)ch->info->scmi_mbx_mem;
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_SYS_PWR_PROTO_ID,
					      SCMI_SYS_PWR_STATE_SET_MSG, 0);
	mbx_mem->len = SCMI_SYS_PWR_STATE_SET_MSG_LEN;
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	SCMI_PAYLOAD_ARG2(mbx_mem->payload, SCMI_SYS_PWR_GRACEFUL_REQ, state);

	SCMI_MARK_CHANNEL_BUSY(mbx_mem->status);

	/*
	 * Ensure the header/payload writes are visible to FSS before the
	 * doorbell write.
	 */
	dmbst();

	ch->info->ring_doorbell(ch->info);

	/*
	 * The following spin_unlock() uses STLR (Store-Release), which orders
	 * the preceding doorbell write before the lock release, so no explicit
	 * barrier is needed here. Intentionally do NOT wait for the channel to
	 * be marked free.
	 */
	spin_unlock(ch->lock);

	return SCMI_E_SUCCESS;
}

static void __dead2 a78_core_turn_off(void)
{
	/* dsb is good practice before using wfi to enter low power states */
	dsb();
	isb();

	while (true) {
		/* power down when executing wfi */
		wfi();
	};
}

int fss_cpu_on(uint32_t core)
{
	if (core >= PLATFORM_CORE_COUNT)
		return -EINVAL;

	return fss_cpu_set_state(core, CPU_ON);
}

void __dead2 fss_cpu_off(uint32_t core)
{
	int ret;

	if (core >= PLATFORM_CORE_COUNT) {
		ERROR("Invalid core ID %" PRIu32 ". Core ID must be less than %d\n",
		      core, PLATFORM_CORE_COUNT);
		panic();
	}

	ret = fss_cpu_set_state(core, CPU_OFF);
	if (ret) {
		ERROR("Failed to set CPU %" PRIu32 " to OFF state. Error=%d\n",
		      core, ret);
		panic();
	}

	a78_core_turn_off();
}

void __dead2 fss_system_suspend(void)
{
	int ret;

	ret = fss_set_sys_pwr_state(SCMI_SYS_PWR_SUSPEND);
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to transition the system to suspend state. Error=%d\n", ret);
		panic();
	}

	a78_core_turn_off();
}

int fss_cohort_reset(void)
{
	int ret;

	ret = fss_set_sys_pwr_state_nowait(SCMI_SYS_PWR_COLD_RESET);
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to Cold Reset the Cohort. Error=%d\n", ret);
	}

	return ret;
}

int fss_cohort_shutdown(void)
{
	int ret;

	ret = fss_set_sys_pwr_state_nowait(SCMI_SYS_PWR_SHUTDOWN);
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to Shutdown the Cohort. Error=%d\n", ret);
	}

	return ret;
}

int fss_scmi_nxp_core_validate(void)
{
	void *handle = get_scmi_handle(SCMI_TX);

	if (!handle) {
		ERROR("Could not retrieve SCMI handle.\n");
		return -EINVAL;
	}

	return scmi_nxp_core_init(handle);
}

int fss_set_cores_reset_addr(uintptr_t core_addr)
{
	void *handle = get_scmi_handle(SCMI_TX);
	unsigned int i;
	int ret = 0;

	if (!handle) {
		ERROR("Could not retrieve SCMI channel handle.\n");
		return -EINVAL;
	}

	for (i = 0; i < get_plat_core_count(); i++) {
		/* Configuring the reset address is
		 * not supported for the primary core
		 */
		if (is_primary_core(i))
			continue;

		ret = scmi_nxp_core_configure_core(handle,
				get_core_nxp_core_id(i), core_addr);
		if (ret)
			return ret;
	}

	return 0;
}

int fss_get_misc_info(void)
{
	void *handle = get_scmi_handle(SCMI_TX);
	int ret;

	if (!handle) {
		ERROR("Could not retrieve SCMI channel handle.\n");
		return -EINVAL;
	}

	ret = scmi_nxp_misc_display_firmware_info(handle);
	if (ret)
		return ret;

	return scmi_nxp_misc_display_data_version(handle);
}

int fss_get_gic_mode(uint8_t *gic_mode)
{
	void *handle = get_scmi_handle(SCMI_TX);

	if (!handle) {
		ERROR("Could not retrieve SCMI channel handle.\n");
		return -EINVAL;
	}

	return scmi_nxp_misc_get_gpo_signal(handle, gic_mode);
}

int fss_display_reset_plat_reason(void)
{
	void *handle = get_scmi_handle(SCMI_TX);

	if (!handle) {
		ERROR("Could not retrieve SCMI channel handle.\n");
		return -EINVAL;
	}

	return scmi_reset_display_plat_reason(handle);
}

int fss_display_reset_cohort_reason(uint32_t cohort_id)
{
	void *handle = get_scmi_handle(SCMI_TX);

	if (!handle) {
		ERROR("Could not retrieve SCMI channel handle.\n");
		return -EINVAL;
	}

	return scmi_nxp_boot_display_reset_reason(handle, cohort_id);
}

int fss_get_clock_rate(uint32_t clk_id, unsigned long *rate)
{
	void *handle = get_scmi_handle(SCMI_TX);

	if (!handle) {
		ERROR("Could not retrieve SCMI channel handle.\n");
		return -EINVAL;
	}

	return scmi_clock_get_rate(handle, clk_id, rate);
}
