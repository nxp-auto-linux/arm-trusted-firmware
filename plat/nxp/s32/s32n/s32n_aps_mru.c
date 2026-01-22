/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <s32_bl_common.h>
#include <s32n_aps_mru.h>

#define SCMI_FSS_RECEIVING_KEY		(0x49504346)

int mru_init(uintptr_t mru_base,
	     uint8_t chan, uint8_t mbox)
{
	uint32_t mbstat_val, mbstat_addr, cfg0_addr;
	uint32_t mbe = BIT_32(MRU_CFG0_MBE_SHIFT + mbox);

	if (!chan) {
		ERROR("MRU Channel index start at value 1\n");
		return -EINVAL;
	}

	cfg0_addr = mru_base + MRU_CHANNEL_OFFSET(chan - 1) +
		MRU_CFG0_OFFSET;
	/*
	 * Enable channel
	 * Enables writes to all other configuration fields in the register.
	 */
	mmio_setbits_32(cfg0_addr, MRU_CFG0_CHE);

	/* Clear any pending flags */
	mbstat_addr = mru_base + MRU_CHANNEL_OFFSET(chan - 1) +
		MRU_MBSTAT_OFFSET;
	mbstat_val = mmio_read_32(mbstat_addr);
	if (mbstat_val)
		mmio_write_32(mbstat_addr, mbstat_val);

	/* Enable Interrupt and Mailbox */
	mmio_setbits_32(cfg0_addr, MRU_CFG0_IE | mbe);

	return 0;
}

int mru_irq_read_data(uintptr_t mru_addr,
		      uint8_t chan, uint8_t mbox)
{
	uint32_t mbstat_val, mbstat_addr, mb_data;
	uint32_t mbx_addr;

	mbstat_addr = mru_addr + MRU_CHANNEL_OFFSET(chan - 1) +
		MRU_MBSTAT_OFFSET;
	mbstat_val = mmio_read_32(mbstat_addr);
	if (!mbstat_val)
		return -EINVAL;

	/* Clear irq */
	mmio_write_32(mbstat_addr, mbstat_val);

	mbx_addr = mru_addr + MRU_MBX_OFFSET + (mbox * 4);
	mb_data = mmio_read_32(mbx_addr);
	if (mb_data != SCMI_FSS_RECEIVING_KEY)
		return -EINVAL;

	return 0;
}
