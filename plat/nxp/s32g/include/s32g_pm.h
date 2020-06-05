/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32G274A_PM_H
#define S32G274A_PM_H

#include <stdint.h>
#include "s32g_clocks.h"

enum periph_clock_id {
	CLKOUT0,
	CLKOUT1,
	PCIE_0_REF_CLK,
	PER_CLK,
	FTM_0_REF_CLK,
	FTM_1_REF_CLK,
	FLEXRAY_PE_CLK,
	CAN_PE_CLK,
	LINFLEXD_CLK__LIN_BAUD_CLK,
	GMAC_TS_CLK,
	GMAC_0_TX_CLK,
	GMAC_0_RX_CLK,
	GMAC_0_REF_DIV_CLK,
	SPI_CLK,
	QSPI_1X_CLK__QSPI_2X_CLK,
	SDHC_CLK
};

struct periph_clock {
	enum periph_clock_id id;
	uintptr_t cgm_mux_dc;
	uintptr_t cgm_mux_div_upd_stat;
	uintptr_t cgm_mux_csc;
	uintptr_t plldig_pllodiv;
	uintptr_t dfs_portreset;
	uint8_t dfs_portreset_bitmask;
};

enum req_clk_state {
	CLK_OFF,
	CLK_ON
};

void periph_clock_ctrl(enum periph_clock_id id,
		       enum req_clk_state req_clk_state);

#endif /* S32G274A_PM_H */
