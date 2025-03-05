/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <errno.h>
#include <platform.h>
#include <drivers/scmi.h>
#include <dt-bindings/power/s32n-scmi-pd.h>
#include <inttypes.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <s32n_fss_mru.h>
#include <s32_bl_common.h>
#include <s32_scmi.h>
#include <s32n_bl_common.h>
#include <s32n_fconf_scmi_getter.h>
#include <s32n_fconf_gic_getter.h>

static const uint32_t core_power_domain_ids[] = {
	S32N_APS0_CORE0,
	S32N_APS0_CORE1,
	S32N_APS1_CORE0,
	S32N_APS1_CORE1,
	S32N_APS2_CORE0,
	S32N_APS2_CORE1,
	S32N_APS3_CORE0,
	S32N_APS3_CORE1,
};

typedef enum {
	CPU_ON,
	CPU_OFF,
} cpu_power_state_t;

#define S32N_SCMI_TIMEOUT_US (10 * 1000000U) /* 10s */

/* Magic value used to ring the FSS MRU doorbell */
#define RING_DB_VAL			0x49504346U

/* Magic number to indicate the messaging channel is initialized */
#define SHM_STATE_READY		0x4950434646435049ULL

static scmi_channel_t scmi_channel;
static scmi_channel_plat_info_t s32n_scmi_plat_info;
static void *scmi_handle;
static spinlock_t s32n_scmi_lock;

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

static uint32_t get_core_domain_id(uint32_t core)
{
	unsigned int pos = get_core_global_id(core);

	assert(pos < ARRAY_SIZE(core_power_domain_ids));
	return core_power_domain_ids[pos];
}

/** Check FSS ready state */
static bool is_fss_ready(void *arg)
{
	uintptr_t remote_addr = get_remote_handshake_addr();

	return mmio_read_64(remote_addr) == SHM_STATE_READY;
}

static int fss_handshake_init(void)
{
	uintptr_t local_addr = get_local_handshake_addr();

	/* write local ready state */
	mmio_write_64(local_addr, SHM_STATE_READY);

	return wait_for_condition_timeout(is_fss_ready,
				NULL, S32N_SCMI_TIMEOUT_US);
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

	ret = fss_handshake_init();
	if (ret) {
		ERROR("FSS SCMI handshake failed\n");
		return NULL;
	}

	ch->is_initialized = 1;
	return ch;
}

static scmi_channel_t *get_scmi_channel(void)
{
	if (!scmi_channel.is_initialized) {
		scmi_handle = scmi_chan_init(&scmi_channel);
		if (!scmi_handle) {
			ERROR("Could not initialize SCMI channel.\n");
			panic();
		}
	}

	return &scmi_channel;
}

static void *get_scmi_handle(void)
{
	scmi_channel_t *ch = get_scmi_channel();

	if (!ch)
		return NULL;

	return scmi_handle;
}

static uintptr_t get_scmi_sh_mem_base(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_sh_mem_config, sh_mem_base);
}

static size_t get_scmi_sh_mem_size(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_sh_mem_config, sh_mem_size);
}

static uintptr_t get_scmi_mb_addr(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_fss_ch_config, mb_base);
}

static size_t get_scmi_mb_size(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_fss_ch_config, mb_size);
}

static unsigned long get_fss_ch_mb0(unsigned int n)
{
	assert(n && n <= FSS_NUM_CHANNELS);
	return FSS_MRU_0_BASE + (n) * FSS_CH_SIZE;
}

static uintptr_t get_db_reg_addr(void)
{
	unsigned int mru_chan = FCONF_GET_PROPERTY(s32n, scmi_fss_ch_config, mru_ch);

	return get_fss_ch_mb0(mru_chan);
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
	scmi_channel_t *ch = get_scmi_channel();
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

void fss_scmi_init(void)
{
	uintptr_t base;
	size_t size;
	int ret;

	s32n_scmi_plat_info = (scmi_channel_plat_info_t) {
		.scmi_mbx_mem = get_scmi_mb_addr(),
		.scmi_mbx_size = get_scmi_mb_size(),
		.db_reg_addr = get_db_reg_addr(),
		.ring_doorbell = &fss_ring_doorbell,
	};

	scmi_channel = (scmi_channel_t) {
		.info = &s32n_scmi_plat_info,
		.lock = &s32n_scmi_lock,
	};

	base = get_scmi_sh_mem_base();
	size = get_scmi_sh_mem_size();
	ret = mmap_add_dynamic_region(base, base, MMU_ROUND_UP_TO_PAGE(size),
				MT_NON_CACHEABLE | MT_RW | MT_SECURE);
	if (ret < 0) {
		ERROR("Error mapping SCMI shared mailbox mem %d\n", ret);
		panic();
	}

	ret = mmap_add_dynamic_region(FSS_MRU_0_BASE, FSS_MRU_0_BASE,
			MMU_ROUND_UP_TO_PAGE(FSS_MRU_0_SIZE),
			MT_DEVICE | MT_RW | MT_SECURE);
	if (ret < 0) {
		ERROR("Error mapping FSS MRU region %d\n", ret);
		panic();
	}
}

static int fss_cpu_set_state(uint32_t core, cpu_power_state_t state)
{
	void *handle = get_scmi_handle();
	uint32_t domain_id = get_core_domain_id(core);
	uint32_t pwr_state = 0;
	int ret;

	if (!handle) {
		ERROR("%s: Failed to get SCMI handle", __func__);
		panic();
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

static void __dead2 a78_core_turn_off(void)
{
	while (true) {
		/* power down when executing wfi */
		isb();
		wfi();
	};
}

int fss_cpu_on(uint32_t core)
{
	if (core >= PLATFORM_CORE_COUNT)
		return -EINVAL;

	if (is_fss_used())
		return fss_cpu_set_state(core, CPU_ON);

	return 0;
}

int fss_cpu_off(uint32_t core)
{
	int ret = 0;

	if (core >= PLATFORM_CORE_COUNT)
		return -EINVAL;

	if (is_fss_used()) {
		ret = fss_cpu_set_state(core, CPU_OFF);
		if (ret)
			return ret;
	}

	a78_core_turn_off();

	return ret;
}

static int scmi_ap_core_init(scmi_channel_t *ch)
{
	uint32_t version, proto_attr, msg_attr;
	unsigned int reset_addr_width;
	int ret;

	/* AP core protocol version */
	ret = scmi_proto_version(ch, SCMI_AP_CORE_PROTO_ID, &version);
	if (ret != SCMI_E_SUCCESS) {
		ERROR("SCMI AP core protocol version message failed\n");
		return -EINVAL;
	}

	if (!is_scmi_version_compatible(SCMI_AP_CORE_PROTO_VER, version)) {
		ERROR("SCMI AP core protocol version 0x%x incompatible with driver version 0x%x\n",
						version, SCMI_AP_CORE_PROTO_VER);
		return -EINVAL;
	}

	/* AP core protocol attributes */
	ret = scmi_ap_core_proto_attr(ch, &proto_attr);
	if (ret != SCMI_E_SUCCESS) {
		ERROR("SCMI AP core protocol attributes is not supported by SCMI\n");
		return -EINVAL;
	}

	reset_addr_width = proto_attr & SCMI_AP_CORE_64_BIT_ADDR_MASK ? 64U : 32U;
	if (reset_addr_width == 32) {
		ERROR("SCMI AP core protocol 32-bit reset address not supported\n");
		return -EINVAL;
	}

	NOTICE("SCMI AP core protocol version 0x%x detected (%u-bit reset address)\n",
			version, reset_addr_width);

	/* AP core protocol message attributes */
	ret = scmi_proto_msg_attr(ch, SCMI_AP_CORE_PROTO_ID,
				SCMI_AP_CORE_RESET_ADDR_SET_MSG, &msg_attr);
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Set reset address command is not supported by SCMI\n");
		return -EINVAL;
	}

	return 0;
}

void fss_scmi_ap_core_init(void)
{
	scmi_channel_t *ch = get_scmi_channel();

	if (!ch) {
		ERROR("Could not retrieve SCMI channel.\n");
		return;
	}

	if (scmi_ap_core_init(ch) < 0) {
		ERROR("SCMI AP core protocol initialization failed.\n");
		panic();
	}
}

void fss_set_core_reset_addr(uintptr_t core_addr)
{
	void *handle = get_scmi_handle();
	int ret;

	if (!handle) {
		ERROR("Could not retrieve SCMI channel handle.\n");
		return;
	}

	ret = scmi_ap_core_set_reset_addr(handle, core_addr, 0x0);
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Could not set core reset address: %d.\n", ret);
		panic();
	}
}
