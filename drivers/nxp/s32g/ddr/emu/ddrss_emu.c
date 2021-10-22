/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <ddr/ddrss.h>
#include <lib/mmio.h>

extern struct ddrss_conf ddrss_conf;
extern struct ddrss_firmware ddrss_firmware;

static void ddrss_init(struct ddrss_conf *ddr_conf,
		       struct ddrss_firmware *ddr_firmware)
{
	uint32_t dfmisc_conf = CTL_IDLE_EN_MASK | DIS_DYN_ADR_TRI_MASK;

	write_regconf_32(ddr_conf->ddrc_conf, ddr_conf->ddrc_conf_length);

	mmio_write_32(REG_GRP0, mmio_read_32(REG_GRP0) | AXI_PARITY_EN_MASK);
	mmio_write_32(REG_GRP0, mmio_read_32(REG_GRP0) | AXI_PARITY_TYPE_MASK);
	mmio_write_32(REG_GRP0, mmio_read_32(REG_GRP0) | DFI1_ENABLED_MASK);

	deassert_ddr_reset();
	mmio_write_32(PWRCTL, 0);
	mmio_write_32(PWRCTL, 0);
	mmio_write_32(SWCTL, 0);

	while (mmio_read_32(SWSTAT) & SW_DONE_ACK_MASK)
		;

	mmio_write_32(DFIMISC, dfmisc_conf);
	mmio_write_32(MRCTRL0, PBA_MODE | MR_ADDR_MR6 |
		      MR_RANK_01 | SW_INIT_INT);
	mmio_write_32(DFIMISC, dfmisc_conf);
	mmio_write_32(MRCTRL0, MR_RANK_01 | SW_INIT_INT);
	mmio_write_32(MRCTRL1, 0xb05);
	mmio_write_32(MRCTRL0, MR_RANK_01 | SW_INIT_INT);

	write_regconf_16(ddr_conf->ddrphy_conf,
			 ddr_conf->ddrphy_conf_length);

	mmio_write_32(DFIMISC, DFI_INIT_START_MASK | DIS_DYN_ADR_TRI_MASK);

	while ((mmio_read_32(DFISTAT) & DFI_INIT_COMPLETE_MASK) !=
	       DFI_INIT_COMPLETE_MASK)
		;

	mmio_write_32(DFIMISC, dfmisc_conf);
	mmio_write_32(DFIMISC, dfmisc_conf | DFI_INIT_COMPLETE_EN_MASK);
	mmio_write_32(DFIMISC, dfmisc_conf | DFI_INIT_COMPLETE_EN_MASK);

	while (mmio_read_32(MRSTAT) & MR_WR_BUSY)
		;

	mmio_write_32(MRCTRL0, MR_RANK_01);
	mmio_write_32(SWCTL, SW_DONE_MASK);

	while ((mmio_read_32(SWSTAT) & SW_DONE_ACK_MASK) != SW_DONE_ACK_MASK)
		;

	while ((mmio_read_32(STAT) & OPERATING_MODE_NORMAL) != OPERATING_MODE_NORMAL)
		;

	mmio_write_32(PWRCTL, 0);
	mmio_write_32(PWRCTL, 0);
	mmio_write_32(PCTRL_0, mmio_read_32(PCTRL_0) | PORT_EN_MASK);
	mmio_write_32(PCTRL_1, mmio_read_32(PCTRL_1) | PORT_EN_MASK);
	mmio_write_32(PCTRL_2, mmio_read_32(PCTRL_2) | PORT_EN_MASK);
}

int ddr_init(void)
{
	ddrss_init(&ddrss_conf, &ddrss_firmware);
	return 0;
}


