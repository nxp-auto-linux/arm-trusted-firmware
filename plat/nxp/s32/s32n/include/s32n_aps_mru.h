/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_APS_MRU_H
#define S32N_APS_MRU_H

#include <platform_def.h>

/* Channel offsets */
#define MRU_CHANNEL_OFFSET(x)   ((x) * 0x10)

#define MRU_MBX_OFFSET			0x10000

/* Register offsets (from channel base) */
#define MRU_CFG0_OFFSET         0x0
#define MRU_MBSTAT_OFFSET       0x8

/* CFG0 register bits */
#define MRU_CFG0_CHE            BIT_32(0)   /* Channel Enable */
#define MRU_CFG0_IE             BIT_32(2)   /* Interrupt Enable */
#define MRU_CFG0_MBE_SHIFT		(16)

/* MBSTAT register bits */
#define MRU_MBSTAT_MBF0         BIT_32(16)  /* Mailbox 0 Flag */

int mru_init(uintptr_t mru_base,
	     uint8_t chan, uint8_t mbox);
int mru_irq_read_data(uintptr_t mru_addr,
		      uint8_t chan, uint8_t mbox);

#endif /* S32N_APS_MRU_H */
