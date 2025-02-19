/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32_DDR_H
#define S32_DDR_H

struct ddr_lp_config {
	uint32_t *csr_to_store;
	size_t csr_to_store_size;
	uint16_t *csr_settings;
	size_t csr_settings_size;
	uint32_t *ddrc_to_store;
	size_t ddrc_to_store_size;
};

extern struct ddr_lp_config ddr_lp_configs[];

#endif /* S32_DDR_H_ */
