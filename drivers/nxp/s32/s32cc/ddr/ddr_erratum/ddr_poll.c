/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "ddr_utils.h"

uint32_t s32_poll_derating_temp_errata(void)
{
	uint32_t nominal_temp_flag = 0;
	uint8_t val_1, val_2;
	uint32_t reg, bf_val;

	if (read_tuf() <= TUF_THRESHOLD) {
		nominal_temp_flag++;

		val_1 = read_tuf();
		val_2 = read_tuf();

		if (val_1 <= TUF_THRESHOLD && val_2 <= TUF_THRESHOLD)
			nominal_temp_flag += SUCCESSIVE_READ;
	}
	if (nominal_temp_flag != REQUIRED_OK_CHECKS)
		return ERRATA_CHANGES_UNMODIFIED;

	/*
	 * Update average time interval between refreshes per rank:
	 * RFSHTMG.T_RFC_NOM_X1_X32 = RFSHTMG.T_RFC_NOM_X1_X32 * 4
	 */
	reg = mmio_read_32(DDRC_BASE + OFFSET_DDRC_RFSHTMG);
	bf_val = (reg >> RFSHTMG_VAL_SHIFT) & RFSHTMG_VAL;
	bf_val = bf_val << RFSHTMG_UPDATE_SHIFT;
	reg = (reg & ~RFSHTMG_MASK) |
		(bf_val << RFSHTMG_VAL_SHIFT);
	mmio_write_32(DDRC_BASE + OFFSET_DDRC_RFSHTMG, reg);

	/*
	 * Toggle RFSHCTL3.REFRESH_UPDATE_LEVEL to indicate that
	 * refresh registers have been updated
	 */
	reg = mmio_read_32(DDRC_BASE + OFFSET_DDRC_RFSHCTL3);
	bf_val = (reg >> RFSHCTL3_UPDATE_SHIFT) &
		RFSHCTL3_AUTO_REFRESH_VAL;
	bf_val = bf_val ^ RFSHCTL3_UPDATE_LEVEL_TOGGLE;
	reg = (reg & ~RFSHCTL3_MASK) |
	      (bf_val << RFSHCTL3_UPDATE_SHIFT);
	mmio_write_32(DDRC_BASE + OFFSET_DDRC_RFSHCTL3, reg);

	/* Enable timing parameter derating: DERATEEN.DERATE_EN = 1 */
	reg = mmio_read_32(DDRC_BASE + OFFSET_DDRC_DERATEEN);
	mmio_write_32(DDRC_BASE + OFFSET_DDRC_DERATEEN,
		      reg | DERATEEN_ENABLE);

	return ERRATA_CHANGES_REVERTED;
}
