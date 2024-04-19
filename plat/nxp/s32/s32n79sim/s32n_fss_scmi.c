/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <platform.h>
#include <drivers/scmi.h>
#include <drivers/arm/css/scmi.h>

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

scmi_channel_t *get_scmi_channel(void)
{
	if (!scmi_channel.is_initialized)
		s32n_scmi_chan_init(&scmi_channel);

	return &scmi_channel;
}

static void fss_ring_doorbell(struct scmi_channel_plat_info *plat_info)
{

}

void fss_scmi_init(void)
{
	s32n_scmi_plat_info = (scmi_channel_plat_info_t) {
		.scmi_mbx_mem = (uintptr_t)NULL,
		.scmi_mbx_size = 0x80,
		.db_reg_addr = (uintptr_t)NULL,
		.ring_doorbell = &fss_ring_doorbell,
	};

	scmi_channel = (scmi_channel_t) {
		.info = &s32n_scmi_plat_info,
		.lock = &s32n_scmi_lock,
	};

}

