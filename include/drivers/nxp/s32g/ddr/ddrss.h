/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DDRSS_H
#define DDRSS_H

#include <stddef.h>
#include <lib/mmio.h>

#define DDRSS_BASE_ADDR			0x40380000

#define DDRSS_DMEM_ADDR			(DDRSS_BASE_ADDR + 0x30000)
#define SEQUENCECTRL			(DDRSS_DMEM_ADDR + 0x20)
#define SEQUENCECTRL_RUN_DEVINIT_MASK	BIT(0)
#define HDTCTRL				(DDRSS_DMEM_ADDR + 0x24)
#define HDTCTRL_FIRMWARE_COMPLETION	(0xff)

#define DWC_DDRPHYA_APBONLY0		(DDRSS_BASE_ADDR + 0x400)
#define MICROCONTMUXSEL			(DWC_DDRPHYA_APBONLY0 + 0x0)
#define MICROCONTMUXSEL_MASK		BIT(0)
#define UCTSHADOWREGS			(DWC_DDRPHYA_APBONLY0 + 0x4)
#define UCTWRITEPROTSHADOW_MASK		BIT(0)
#define DCTWRITEPROT			(DWC_DDRPHYA_APBONLY0 + 0xc)
#define DCTWRITEPROT_MASK		BIT(0)
#define UCTWRITEONLYSHADOW		(DWC_DDRPHYA_APBONLY0 + 0x10)
#define MICRORESET			(DWC_DDRPHYA_APBONLY0 + 0x20)
#define RESETTOMICRO_MASK		BIT(3)
#define STALLTOMICRO_MASK		BIT(0)

#define DWC_DDRPHYA_DRTUB0		(DDRSS_BASE_ADDR + 0xbd0)
#define UCTWRITEPROT			(DWC_DDRPHYA_DRTUB0 + 0xc)
#define UCTWRITEPROT_MASK		BIT(0)
#define UCCLKHCLKENABLES		(DWC_DDRPHYA_DRTUB0 + 0x1c)
#define HCLKEN_MASK			BIT(1)
#define UCCLKEN_MASK			BIT(0)

#define DWC_DDRPHYA_INITENG0		(DDRSS_BASE_ADDR + 0xc04)
#define SEQ0BDISABLEFLAG6		(DWC_DDRPHYA_INITENG0 + 0x110)
#define PHYINLP3			(DWC_DDRPHYA_INITENG0 + 0x1b4)
#define PHYINLP3_MASK			BIT(0)

#define DWC_DDRPHYA_MASTER0		(DDRSS_BASE_ADDR + 0x12d8)
#define PPTTRAINSETUP_P0		(DWC_DDRPHYA_MASTER0 + 0x9c)
#define PPTTRAINSETUP2_P0		(DWC_DDRPHYA_MASTER0 + 0xb0)
#define MEMRESETL			(DWC_DDRPHYA_MASTER0 + 0x270)
#define CALBUSY				(DWC_DDRPHYA_MASTER0 + 0x384)
#define CALBUSY_MASK			BIT(0)
#define PLLCTRL1_P0			(DWC_DDRPHYA_MASTER0 + 0x418)
#define PLLCPINTCTRL_MASK		(0x1f)
#define PLLCPINTCTRL_OFFSET		(0)
#define PLLCPPROPCTRL_MASK		(0xf)
#define PLLCPPROPCTRL_OFFSET		(5)
#define PLLTESTMODE_P0			(DWC_DDRPHYA_MASTER0 + 0x430)
#define PLLCTRL4_P0			(DWC_DDRPHYA_MASTER0 + 0x444)
#define PLLCPINTGSCTRL_MASK		(0x1f)
#define PLLCPINTGSCTRL_OFFSET		(0)
#define PLLCPPROPGSCTRL_MASK		(0xf)
#define PLLCPPROPGSCTRL_OFFSET		(5)

#define UMCTL2_REGS			(DDRSS_BASE_ADDR + 0x40000)
#define STAT				(UMCTL2_REGS + 0x4)
#define OPERATING_MODE_MASK		(BIT(0) | BIT(1) | BIT(2))
#define OPERATING_MODE_NORMAL		(0x1)
#define OPERATING_MODE_SELF_REFRESH	(0x3)
#define SELFREF_TYPE_MASK		(BIT(4) | BIT(5))
#define SELFREF_TYPE_NOT_UNDER_AUTO_SR_CTRL	(0x2 << 4)
#define SELFREF_STATE_MASK		(BIT(8) | BIT(9))
#define SELFREF_STATE_SRPD		(0x2 << 8)
#define MRCTRL0				(UMCTL2_REGS + 0x10)
#define PBA_MODE			BIT(30)
#define MR_ADDR_MR6			(6U << 12U)
#define MR_RANK_01			(3U << 4U)
#define SW_INIT_INT			BIT(3)
#define MRCTRL1				(UMCTL2_REGS + 0x14)
#define MRSTAT				(UMCTL2_REGS + 0x18)
#define MR_WR_BUSY			BIT(0)
#define PWRCTL				(UMCTL2_REGS + 0x30)
#define SELFREF_EN_MASK			BIT(0)
#define POWERDOWN_EN_MASK		BIT(1)
#define EN_DFI_DRAM_CLK_DISABLE_MASK	BIT(3)
#define SELFREF_SW_MASK			BIT(5)
#define RFSHCTL3			(UMCTL2_REGS + 0x60)
#define DIS_AUTO_REFRESH_MASK		BIT(0)
#define INIT0				(UMCTL2_REGS + 0xd0)
#define SKIP_DRAM_INIT_MASK		(BIT(30) | BIT(31))
#define DFIMISC				(UMCTL2_REGS + 0x1b0)
#define DFI_FREQUENCY(f)		(f << 8)
#define DFI_INIT_COMPLETE_EN_MASK	BIT(0)
#define CTL_IDLE_EN_MASK		BIT(4)
#define DFI_INIT_START_MASK		BIT(5)
#define DIS_DYN_ADR_TRI_MASK		BIT(6)
#define DFISTAT				(UMCTL2_REGS + 0x1bc)
#define DFI_INIT_COMPLETE_MASK		BIT(0)
#define DBG1				(UMCTL2_REGS + 0x304)
#define SWCTL				(UMCTL2_REGS + 0x320)
#define SW_DONE_MASK			BIT(0)
#define SWSTAT				(UMCTL2_REGS + 0x324)
#define SW_DONE_ACK_MASK		BIT(0)
#define ADDRMAP0			(UMCTL2_REGS + 0x200)
#define ADDRMAP_CS_BIT0_MASK		(0x1f)
#define ADDRMAP_CS_BIT0_OFFSET		(0)
#define ADDRMAP6			(UMCTL2_REGS + 0x218)
#define ADDRMAP_ROW_B12_MASK		(0xf)
#define ADDRMAP_ROW_B12_OFFSET		(0)
#define ADDRMAP_ROW_B13_MASK		(0xf)
#define ADDRMAP_ROW_B13_OFFSET		(8)
#define ADDRMAP_ROW_B14_MASK		(0xf)
#define ADDRMAP_ROW_B14_OFFSET		(16)
#define ADDRMAP_ROW_B15_MASK		(0xf)
#define ADDRMAP_ROW_B15_OFFSET		(24)
#define ADDRMAP7			(UMCTL2_REGS + 0x21c)
#define ADDRMAP_ROW_B16_MASK		(0xf)
#define ADDRMAP_ROW_B16_OFFSET		(0)

#define UMCTL2_MP			(DDRSS_BASE_ADDR + 0x403f8)
#define PSTAT				(UMCTL2_MP + 0x4)
#define PCTRL_0				(UMCTL2_MP + 0x98)
#define PCTRL_1				(UMCTL2_MP + 0x148)
#define PCTRL_2				(UMCTL2_MP + 0x1f8)
#define PORT_EN_MASK			BIT(0)
#define SBRCTL				(UMCTL2_MP + 0xb2c)
#define SCRUB_EN_MASK			BIT(0)
#define SBRSTAT				(UMCTL2_MP + 0xb30)
#define SCRUB_BUSY_MASK			BIT(0)

#define DDR_SUBSYSTEM			(DDRSS_BASE_ADDR + 0x50000)
#define REG_GRP0			(DDR_SUBSYSTEM + 0x0)
#define AXI_PARITY_EN_MASK		(0x1ff0)
#define AXI_PARITY_EN(e)		((e << 4) & AXI_PARITY_EN_MASK)
#define AXI_PARITY_TYPE_MASK		(0x1ff0000)
#define AXI_PARITY_TYPE(t)		((t << 16) & AXI_PARITY_TYPE_MASK)
#define DFI1_ENABLED_MASK		BIT(0)

#define MAIL_TRAINING_SUCCESS		(0x07)

#define DDR_GPR				(0x4007c600ul)
#define DDR_RET_CONTROL			(DDR_GPR + 0x1c)
#define DDR_RET_CONTROL_MASK		BIT(0)

struct regconf {
	uint32_t addr;
	uint32_t data;
};

struct ddrss_conf {
	struct regconf *ddrc_conf;
	size_t ddrc_conf_length;
	struct regconf *ddrphy_conf;
	size_t ddrphy_conf_length;
	struct regconf *pie;
	size_t pie_length;
	struct regconf *message_block_1d;
	size_t message_block_1d_length;
};

struct ddrss_firmware {
	struct regconf *imem_1d;
	size_t imem_1d_length;
	struct regconf *dmem_1d;
	size_t dmem_1d_length;
};

void ddrss_init(struct ddrss_conf *ddrss_conf,
		struct ddrss_firmware *ddrss_firmware, uintptr_t csr_array);
void ddrss_to_io_lp3_retention_mode(void);
void ddrss_to_normal_mode(struct ddrss_conf *ddrss_conf, uintptr_t csr_array);

#endif
