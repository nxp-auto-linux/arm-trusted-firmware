/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <common/debug.h>
#include <drivers/arm/css/scmi.h>
#include <lib/mmio.h>
#include <libc/assert.h>

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
