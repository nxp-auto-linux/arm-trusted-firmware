/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <errno.h>
#include <platform.h>
#include <drivers/scmi.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <s32n_fss_mru.h>
#include <s32_bl_common.h>
#include <s32_scmi.h>
#include <s32n_bl_common.h>
#include <s32n_fconf_scmi_getter.h>

#define S32N_SCMI_TIMEOUT_US (10 * 1000000U) /* 10s */

/* Magic value used to ring the FSS MRU doorbell */
#define RING_DB_VAL			0x49504346U

/* Magic number to indicate the messaging channel is initialized */
#define SHM_STATE_READY		0x4950434646435049ULL

static scmi_channel_t scmi_channel;
static scmi_channel_plat_info_t s32n_scmi_plat_info;
static spinlock_t s32n_scmi_lock;

static uintptr_t get_local_handshake_addr(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_sh_mem_config, local_handshake_addr);
}

static uintptr_t get_remote_handshake_addr(void)
{
	return FCONF_GET_PROPERTY(s32n, scmi_sh_mem_config, remote_handshake_addr);
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

static void scmi_chan_init(scmi_channel_t *ch)
{
	int ret;

	assert(ch && ch->info);
	assert(ch->info->ring_doorbell != NULL);
	assert(ch->lock);
	assert(ch->info->db_reg_addr);

	ch->is_initialized = 0;

	if (ch->info->scmi_mbx_mem) {
		ret = fss_handshake_init();
		if (ret)
			ERROR("SCMI channel not initialized.");
		else
			ch->is_initialized = 1;
	}
}

static scmi_channel_t *get_scmi_channel(void)
{
	if (!scmi_channel.is_initialized)
		scmi_chan_init(&scmi_channel);

	if (!scmi_channel.is_initialized)
		return NULL;

	return &scmi_channel;
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

	ret = wait_for_condition_timeout(is_scmi_channel_free,
			(void *)info->scmi_mbx_mem, S32N_SCMI_TIMEOUT_US);
	if (ret)
		return SCMI_COMMS_ERROR;

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

	/* Copy the request to the mailbox */
	ret = copy_scmi_msg(info->scmi_mbx_mem, scmi_mem, info->scmi_mbx_size);
	if (ret)
		return SCMI_OUT_OF_RANGE;

	/* Ring doorbell */
	dmbst();
	ch->info->ring_doorbell(ch->info);
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

	return 0;
}

int fss_cpu_off(uint32_t core)
{
	if (core >= PLATFORM_CORE_COUNT)
		return -EINVAL;

	a78_core_turn_off();

	return 0;
}
