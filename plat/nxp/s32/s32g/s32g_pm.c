/*
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "s32g_pm.h"
#include <arch_helpers.h>
#include <errno.h>
#include <lib/mmio.h>
#include <stdbool.h>

static const struct periph_clock periph_clocks[] = {
	{.id = CLKOUT0,
	 .cgm_mux_dc =			CGM0_MUXn_DCn(1, 0),
	 .cgm_mux_div_upd_stat =	CGM0_MUXn_DIV_UPD_STAT(1),
	 .cgm_mux_csc =			CGM0_MUXn_CSC(1),
	 .plldig_pllodiv =		PLLDIG_PLLODIV(S32_PERIPH_PLL, 0),
	 .dfs_portreset =		DFS_PORTRESET(S32_PERIPH_DFS),
	 .dfs_portreset_bitmask =	BIT(1) | BIT(4)
	},
	{.id = CLKOUT1,
	 .cgm_mux_dc =			CGM0_MUXn_DCn(2, 0),
	 .cgm_mux_div_upd_stat =	CGM0_MUXn_DIV_UPD_STAT(2),
	 .cgm_mux_csc =			CGM0_MUXn_CSC(2),
	 .plldig_pllodiv =		PLLDIG_PLLODIV(S32_PERIPH_PLL, 0),
	 .dfs_portreset =		DFS_PORTRESET(S32_PERIPH_DFS),
	 .dfs_portreset_bitmask =	BIT(1) | BIT(4)
	},
	{.id = PCIE_0_REF_CLK,
	 .cgm_mux_dc =			(uintptr_t)NULL,
	 .cgm_mux_csc =			(uintptr_t)NULL,
	 .plldig_pllodiv =		PLLDIG_PLLODIV(S32_PERIPH_PLL, 0),
	 .dfs_portreset =		(uintptr_t)NULL
	},
	{.id = PER_CLK,
	 .cgm_mux_dc =			CGM0_MUXn_DCn(3, 0),
	 .cgm_mux_div_upd_stat =	CGM0_MUXn_DIV_UPD_STAT(3),
	 .cgm_mux_csc =			CGM0_MUXn_CSC(3),
	 .plldig_pllodiv =		PLLDIG_PLLODIV(S32_PERIPH_PLL, 1),
	 .dfs_portreset =		(uintptr_t)NULL
	},
	{.id = FTM_0_REF_CLK,
	 .cgm_mux_dc =			CGM0_MUXn_DCn(4, 0),
	 .cgm_mux_div_upd_stat =	CGM0_MUXn_DIV_UPD_STAT(4),
	 .cgm_mux_csc =			CGM0_MUXn_CSC(4),
	 .plldig_pllodiv =		PLLDIG_PLLODIV(S32_PERIPH_PLL, 1),
	 .dfs_portreset =		(uintptr_t)NULL
	},
	{.id = FTM_1_REF_CLK,
	 .cgm_mux_dc =			CGM0_MUXn_DCn(5, 0),
	 .cgm_mux_div_upd_stat =	CGM0_MUXn_DIV_UPD_STAT(5),
	 .cgm_mux_csc =			CGM0_MUXn_CSC(5),
	 .plldig_pllodiv =		PLLDIG_PLLODIV(S32_PERIPH_PLL, 1),
	 .dfs_portreset =		(uintptr_t)NULL
	},
	{.id = FLEXRAY_PE_CLK,
	 .cgm_mux_dc =			CGM0_MUXn_DCn(6, 0),
	 .cgm_mux_div_upd_stat =	CGM0_MUXn_DIV_UPD_STAT(6),
	 .cgm_mux_csc =			CGM0_MUXn_CSC(6),
	 .plldig_pllodiv =		PLLDIG_PLLODIV(S32_PERIPH_PLL, 1),
	 .dfs_portreset =		(uintptr_t)NULL
	},
	{.id = CAN_PE_CLK,
	 .cgm_mux_dc =			(uintptr_t)NULL,
	 .cgm_mux_csc =			CGM0_MUXn_CSC(7),
	 .plldig_pllodiv =		PLLDIG_PLLODIV(S32_PERIPH_PLL, 2),
	 .dfs_portreset =		(uintptr_t)NULL
	},
	{.id = LINFLEXD_CLK__LIN_BAUD_CLK,
	 .cgm_mux_dc =			(uintptr_t)NULL,
	 .cgm_mux_csc =			CGM0_MUXn_CSC(8),
	 .plldig_pllodiv =		PLLDIG_PLLODIV(S32_PERIPH_PLL, 3),
	 .dfs_portreset =		(uintptr_t)NULL
	},
	{.id = GMAC_TS_CLK,
	 .cgm_mux_dc =			CGM0_MUXn_DCn(9, 0),
	 .cgm_mux_div_upd_stat =	CGM0_MUXn_DIV_UPD_STAT(9),
	 .cgm_mux_csc =			CGM0_MUXn_CSC(9),
	 .plldig_pllodiv =		PLLDIG_PLLODIV(S32_PERIPH_PLL, 4),
	 .dfs_portreset =		(uintptr_t)NULL
	},
	{.id = GMAC_0_TX_CLK,
	 .cgm_mux_dc =			CGM0_MUXn_DCn(10, 0),
	 .cgm_mux_div_upd_stat =	CGM0_MUXn_DIV_UPD_STAT(10),
	 .cgm_mux_csc =			CGM0_MUXn_CSC(10),
	 .plldig_pllodiv =		PLLDIG_PLLODIV(S32_PERIPH_PLL, 5),
	 .dfs_portreset =		(uintptr_t)NULL
	},
	{.id = GMAC_0_RX_CLK,
	 .cgm_mux_dc =			(uintptr_t)NULL,
	 .cgm_mux_csc =			CGM0_MUXn_CSC(11),
	 .plldig_pllodiv =		(uintptr_t)NULL,
	 .dfs_portreset =		(uintptr_t)NULL
	},
	{.id = GMAC_0_REF_DIV_CLK,
	 .cgm_mux_dc =			CGM0_MUXn_DCn(15, 0),
	 .cgm_mux_div_upd_stat =	CGM0_MUXn_DIV_UPD_STAT(15),
	 .cgm_mux_csc =			CGM0_MUXn_CSC(15),
	 .plldig_pllodiv =		(uintptr_t)NULL,
	 .dfs_portreset =		(uintptr_t)NULL
	},
	{.id = SPI_CLK,
	 .cgm_mux_dc =			(uintptr_t)NULL,
	 .cgm_mux_csc =			CGM0_MUXn_CSC(16),
	 .plldig_pllodiv =		PLLDIG_PLLODIV(S32_PERIPH_PLL, 7),
	 .dfs_portreset =		(uintptr_t)NULL
	},
	{.id = QSPI_1X_CLK__QSPI_2X_CLK,
	 .cgm_mux_dc =			CGM0_MUXn_DCn(12, 0),
	 .cgm_mux_div_upd_stat =	CGM0_MUXn_DIV_UPD_STAT(12),
	 .cgm_mux_csc =			CGM0_MUXn_CSC(12),
	 .plldig_pllodiv =		(uintptr_t)NULL,
	 .dfs_portreset =		DFS_PORTRESET(S32_PERIPH_DFS),
	 .dfs_portreset_bitmask =	BIT(0)
	},
	{.id = SDHC_CLK,
	 .cgm_mux_dc =			CGM0_MUXn_DCn(14, 0),
	 .cgm_mux_div_upd_stat =	CGM0_MUXn_DIV_UPD_STAT(14),
	 .cgm_mux_csc =			CGM0_MUXn_CSC(14),
	 .plldig_pllodiv =		(uintptr_t)NULL,
	 .dfs_portreset =		DFS_PORTRESET(S32_PERIPH_DFS),
	 .dfs_portreset_bitmask =	BIT(2)
	},
};

static size_t n_periph_clocks = ARRAY_SIZE(periph_clocks);

static void periph_clock_cgmdiv_ctrl(const struct periph_clock *periph_clock,
				     enum req_clk_state req_clk_state)
{
	uint32_t regdata;

	if (!periph_clock->cgm_mux_dc)
		return;

	regdata = mmio_read_32(periph_clock->cgm_mux_dc);

	if (req_clk_state == CLK_ON)
		regdata |= MUXn_DCn_DE;
	else
		regdata &= (~MUXn_DCn_DE);

	mmio_write_32(periph_clock->cgm_mux_dc, regdata);

	while (mmio_read_32(periph_clock->cgm_mux_div_upd_stat)
							& DIV_UPD_STAT_DIV_STAT)
		;
}

static bool is_pllodiv_in_use_by(const struct periph_clock *periph_clock,
				 uintptr_t pllodiv)
{
	uint32_t regdata;

	if (periph_clock->plldig_pllodiv != pllodiv)
		return false;

	if (periph_clock->cgm_mux_csc) {
		regdata = mmio_read_32(periph_clock->cgm_mux_csc);
		regdata &= MC_CGM_MUXn_CSC_SELCTL_MASK;
		regdata >>= MC_CGM_MUXn_CSC_SELCTL_OFFSET;
		if (regdata < MC_CGM_MUXn_CSC_SEL_PERIPH_PLL_PHI0
			|| regdata > MC_CGM_MUXn_CSC_SEL_PERIPH_PLL_PHI7)
			return false;
	}

	if (periph_clock->cgm_mux_dc)
		if (!(mmio_read_32(periph_clock->cgm_mux_dc) & MUXn_DCn_DE))
			return false;

	return true;
}

static void periph_clock_plldiv_ctrl(const struct periph_clock *periph_clock,
				     enum req_clk_state req_clk_state)
{
	int i;
	uint32_t regdata;

	if (!periph_clock->plldig_pllodiv)
		return;

	for (i = 0; i < n_periph_clocks; i++) {
		if (periph_clocks[i].id == periph_clock->id)
			continue;
		if (is_pllodiv_in_use_by(&periph_clocks[i],
					 periph_clock->plldig_pllodiv))
			return;
	}

	regdata = mmio_read_32(periph_clock->plldig_pllodiv);

	if (req_clk_state == CLK_ON)
		regdata |= PLLDIG_PLLODIV_DE;
	else
		regdata &= (~PLLDIG_PLLODIV_DE);

	mmio_write_32(periph_clock->plldig_pllodiv, regdata);
}

static void periph_clock_dfsport_ctrl(const struct periph_clock *periph_clock,
				      enum req_clk_state req_clk_state)
{
	uint32_t regdata;

	if (!periph_clock->dfs_portreset)
		return;

	regdata = mmio_read_32(periph_clock->dfs_portreset);

	if (req_clk_state == CLK_ON)
		regdata &= (~periph_clock->dfs_portreset_bitmask);
	else
		regdata |= periph_clock->dfs_portreset_bitmask;

	mmio_write_32(periph_clock->dfs_portreset, regdata);
}

void periph_clock_ctrl(enum periph_clock_id id,
		       enum req_clk_state req_clk_state)
{
	int i;
	const struct periph_clock *periph_clock = NULL;

	for (i = 0; i < n_periph_clocks; i++)
		if (periph_clocks[i].id == id) {
			periph_clock = &periph_clocks[i];
			break;
		}

	if (req_clk_state == CLK_OFF) {
		periph_clock_cgmdiv_ctrl(periph_clock, CLK_OFF);
		periph_clock_plldiv_ctrl(periph_clock, CLK_OFF);
		periph_clock_dfsport_ctrl(periph_clock, CLK_OFF);
		return;
	}

	periph_clock_dfsport_ctrl(periph_clock, CLK_ON);
	periph_clock_plldiv_ctrl(periph_clock, CLK_ON);
	periph_clock_cgmdiv_ctrl(periph_clock, CLK_ON);
}
