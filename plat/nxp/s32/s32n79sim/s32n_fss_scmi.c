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
#include <s32_bl_common.h>
#include <s32_scmi.h>
#include <s32n_bl_common.h>
#include <s32n_fconf_scmi_getter.h>

#define S32N_SCMI_TIMEOUT_US (100U)

static scmi_channel_t scmi_channel;
static scmi_channel_plat_info_t s32n_scmi_plat_info;
static spinlock_t s32n_scmi_lock;

static void s32n_scmi_chan_init(scmi_channel_t *ch)
{
	assert(ch && ch->info);
	assert(ch->info->ring_doorbell != NULL);
	assert(ch->lock);

	ch->is_initialized = 1;
}

static scmi_channel_t *get_scmi_channel(void)
{
	if (!scmi_channel.is_initialized)
		s32n_scmi_chan_init(&scmi_channel);

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

static void fss_ring_doorbell(struct scmi_channel_plat_info *plat_info)
{

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

	if (!ch || !ch->info)
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
		.db_reg_addr = (uintptr_t)NULL,
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
