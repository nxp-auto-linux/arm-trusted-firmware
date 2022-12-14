/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>

#include <drivers/delay_timer.h>
#include <drivers/mmc.h>
#include <lib/mmio.h>
#include "s32g_clocks.h"

#define USDHC				(0x402f0000ull)

#define USDHC_DS_ADDR			(USDHC + 0x0)
#define USDHC_BLK_ATT			(USDHC + 0x4)
#define BLK_ATT_BLKCNT(x)		(((x) & 0xffff) << 16)
#define BLKCNT_FROM_BLK_ATT(r)		(((r) >> 16) & 0xffff)
#define BLK_ATT_BLKSIZE(x)		((x) & 0x1fff)

#define USDHC_CMDARG			(USDHC + 0x8)
#define USDHC_CMD_XFR_TYP		(USDHC + 0xc)
#define CMD_XFR_TYP_RSPTYP(x)		(((x) & 0x3) << 16)
#define CMD_XFR_TYP_RSPTYP_136		CMD_XFR_TYP_RSPTYP(0x1)
#define CMD_XFR_TYP_RSPTYP_48		CMD_XFR_TYP_RSPTYP(0x2)
#define CMD_XFR_TYP_RSPTYP_48_BUSY	CMD_XFR_TYP_RSPTYP(0x3)
#define CMD_XFR_TYP_DPSEL		BIT(21)
#define CMD_XFR_TYP_CICEN		BIT(20)
#define CMD_XFR_TYP_CCCEN		BIT(19)
#define CMD_XFR_TYP_CMDINX(x)		(((x) & 0x3f) << 24)
#define CMDINX_FROM_CMD_XFR_TYP(r)	(((r) >> 24) & 0x3f)
#define USDHC_CMD_RSP(i)		(USDHC + 0x10 + (i) * 0x4)

#define USDHC_PRES_STATE		(USDHC + 0x24)
#define PRES_STATE_CIHB			BIT(0)
#define PRES_STATE_CDIHB		BIT(1)
#define PRES_STATE_DLA			BIT(2)

#define USDHC_PROT_CTRL			(USDHC + 0x28)
#define PROT_CTRL_EMODE_LE		BIT(5)
#define PROT_CTRL_DTW_4			BIT(1)
#define PROT_CTRL_DTW_8			BIT(2)
#define PROT_CTRL_DTW_MASK		(0x6)

#define USDHC_SYS_CTRL			(USDHC + 0x2c)
#define SYS_CTRL_RSTD			BIT(26)
#define SYS_CTRL_RSTC			BIT(25)
#define SYS_CTRL_RSTA			BIT(24)
#define SYS_CTRL_DTOCV(x)		(((x) & 0xf) << 16)
#define SYS_CTRL_DTOCV_MASK		(SYS_CTRL_DTOCV(0xf))
#define SYS_CTRL_SDCLKFS(x)		(((x) & 0xff) << 8)
#define SYS_CTRL_SDCLKFS_MASK		(SYS_CTRL_SDCLKFS(0xff))
#define SYS_CTRL_DVS(x)			(((x) & 0xf) << 4)
#define SYS_CTRL_DVS_MASK		(SYS_CTRL_DVS(0xf))

#define USDHC_INT_STATUS		(USDHC + 0x30)
#define INT_STATUS_CLEAR_ALL		(0xffffffff)
#define INT_STATUS_DMAE			BIT(28)
#define INT_STATUS_DEBE			BIT(22)
#define INT_STATUS_DCE			BIT(21)
#define INT_STATUS_DTOE			BIT(20)
#define INT_STATUS_CIE			BIT(19)
#define INT_STATUS_CEBE			BIT(18)
#define INT_STATUS_CCE			BIT(17)
#define INT_STATUS_CTOE			BIT(16)
#define INT_STATUS_DINT			BIT(3)
#define INT_STATUS_TC			BIT(1)
#define INT_STATUS_CC			BIT(0)
#define INT_STATUS_CMD_ERROR		(INT_STATUS_CIE | INT_STATUS_CEBE | \
					 INT_STATUS_CCE | INT_STATUS_CTOE)
#define INT_STATUS_DATA_ERROR		(INT_STATUS_DMAE | INT_STATUS_DEBE | \
					 INT_STATUS_DCE | INT_STATUS_DTOE)
#define USDHC_INT_STATUS_EN		(USDHC + 0x34)
#define INT_STATUS_EN_BRRSEN		BIT(5)
#define INT_STATUS_EN_BWRSEN		BIT(4)
#define USDHC_INT_SIGNAL_EN		(USDHC + 0x38)

#define USDHC_MIX_CTRL			(USDHC + 0x48)
#define MIX_CTRL_MSBSEL			BIT(5)
#define MIX_CTRL_DTDSEL			BIT(4)
#define MIX_CTRL_DDR_EN			BIT(3)
#define MIX_CTRL_BCEN			BIT(1)
#define MIX_CTRL_DMAEN			BIT(0)
/* Used to reset command specific options left
 * over from the previous one.
 */
#define MIX_CTRL_RESET_MASK		\
	~(MIX_CTRL_MSBSEL | MIX_CTRL_DTDSEL | MIX_CTRL_BCEN | MIX_CTRL_DMAEN)

#define USDHC_DLL_CTRL			(USDHC + 0x60)
#define USDHC_CLK_TUNE_CTRL_STATUS	(USDHC + 0x68)
#define USDHC_MMC_BOOT			(USDHC + 0xc4)

#define USDHC_VEND_SPEC			(USDHC + 0xc0)
#define VEND_SPEC_INIT			(0x20007809)
#define VEND_SPEC_PER_CLKEN		BIT(13)
#define VEND_SPEC_CARD_CLKEN		BIT(14)

/* These masks represent the commands which involve a data transfer. But since
 * MMC_CMD(x) and MMC_ACMD(x) are identical, yet they represent different
 * commands, we'll have to read the last command index written to CMD_XFR_TYP.
 * If the last command sent to the controller is 55, then the current command
 * is an application command, otherwise, it is a standard command.
 */

#ifdef S32G_BOOT_FROM_EMMC
#define MMC_CMD_ADTC_MASK		(BIT(18) | BIT(17))
#else
#define MMC_CMD_ADTC_MASK		(BIT(6) | BIT(18) | BIT(17))
#endif

#define MMC_ACMD_ADTC_MASK		(BIT(51))
#define ADTC_MASK_FROM_CMD_XFR_TYP(r)	\
			((CMDINX_FROM_CMD_XFR_TYP(r) == MMC_CMD(55)) ? \
			 MMC_ACMD_ADTC_MASK : MMC_CMD_ADTC_MASK)

#define IDENTIFICATION_MODE_FREQUENCY	(400 * 1000)
#define MMC_FULL_SPEED_MODE_FREQUENCY	(26 * 1000 * 1000)

#ifdef S23G_BOOT_FROM_EMMC
static struct mmc_device_info device_info = {
	.mmc_dev_type = MMC_IS_EMMC,
};
#else
static struct mmc_device_info device_info = {
	.mmc_dev_type = MMC_IS_SD,
	.ocr_voltage = OCR_3_2_3_3 | OCR_3_3_3_4,
};
#endif

static uint32_t prepare_ds_addr;
static uint32_t prepare_blk_att;

static void s32g274a_mmc_set_clk(uint64_t clk)
{
	uint32_t regdata;
	int prediv = 1;
	int div = 1;

	assert(!(mmio_read_32(USDHC_MIX_CTRL) & MIX_CTRL_DDR_EN));

	while (SDHC_CLK_FREQ / (prediv * 16) > clk && prediv < 256)
		prediv <<= 1;
	while (SDHC_CLK_FREQ / (prediv * div) > clk && div < 16)
		div++;
	prediv >>= 1;
	div--;

	regdata = mmio_read_32(USDHC_VEND_SPEC) & ~(VEND_SPEC_CARD_CLKEN);
	mmio_write_32(USDHC_VEND_SPEC, regdata);

	regdata = mmio_read_32(USDHC_SYS_CTRL);
	regdata &= ~(SYS_CTRL_SDCLKFS_MASK | SYS_CTRL_DVS_MASK);
	regdata |= SYS_CTRL_SDCLKFS(prediv) | SYS_CTRL_DVS(div);
	mmio_write_32(USDHC_SYS_CTRL, regdata);

	udelay(10 * 1000);

	mmio_write_32(USDHC_VEND_SPEC,
		      mmio_read_32(USDHC_VEND_SPEC) |
		      VEND_SPEC_PER_CLKEN | VEND_SPEC_CARD_CLKEN);
}

static void s32g274a_mmc_init(void)
{
	uint32_t regdata;

	regdata = mmio_read_32(USDHC_SYS_CTRL) | SYS_CTRL_RSTA;
	mmio_write_32(USDHC_SYS_CTRL, regdata);
	while (mmio_read_32(USDHC_SYS_CTRL) & SYS_CTRL_RSTA)
		;

	mmio_write_32(USDHC_MMC_BOOT, 0);
	mmio_write_32(USDHC_MIX_CTRL, 0);
	mmio_write_32(USDHC_CLK_TUNE_CTRL_STATUS, 0);
	mmio_write_32(USDHC_VEND_SPEC, VEND_SPEC_INIT);
	mmio_write_32(USDHC_DLL_CTRL, 0);

	s32g274a_mmc_set_clk(IDENTIFICATION_MODE_FREQUENCY);

	regdata = 0xffffffff & ~(INT_STATUS_EN_BRRSEN | INT_STATUS_EN_BWRSEN);
	mmio_write_32(USDHC_INT_STATUS_EN, regdata);

	mmio_write_32(USDHC_PROT_CTRL, PROT_CTRL_EMODE_LE);

	regdata = mmio_read_32(USDHC_SYS_CTRL);
	regdata &= ~SYS_CTRL_DTOCV_MASK;
	regdata |= SYS_CTRL_DTOCV(0xe);
	mmio_write_32(USDHC_SYS_CTRL, regdata);
}

static int s32g274a_mmc_send_cmd(struct mmc_cmd *cmd)
{
	int i;
	uint32_t cmd_xfr_typ = 0;
	uint32_t mix_ctrl = 0;
	uint32_t cmd_rsp[4];
	uint32_t regdata;
	uint64_t adtc_mask;

	mmio_write_32(USDHC_INT_STATUS, INT_STATUS_CLEAR_ALL);
	while (mmio_read_32(USDHC_PRES_STATE) &
	       (PRES_STATE_CDIHB | PRES_STATE_CIHB | PRES_STATE_DLA))
		;
	mmio_write_32(USDHC_INT_SIGNAL_EN, 0);

	cmd_xfr_typ = CMD_XFR_TYP_CMDINX(cmd->cmd_idx);
	mix_ctrl = mmio_read_32(USDHC_MIX_CTRL);
	mix_ctrl &= MIX_CTRL_RESET_MASK;

	switch (cmd->resp_type) {
	case MMC_RESPONSE_R2:
		cmd_xfr_typ |= CMD_XFR_TYP_RSPTYP_136;
		cmd_xfr_typ |= CMD_XFR_TYP_CCCEN;
		break;
	/* MMC_RESPONSE_R4 == MMC_RESPONSE_R3 */
	case MMC_RESPONSE_R4:
		cmd_xfr_typ |= CMD_XFR_TYP_RSPTYP_48;
		break;
	/* MMC_RESPONSE_R6 == MMC_RESPONSE_R5 == MMC_RESPONSE_R1 */
	case MMC_RESPONSE_R6:
		cmd_xfr_typ |= CMD_XFR_TYP_RSPTYP_48;
		cmd_xfr_typ |= CMD_XFR_TYP_CICEN;
		cmd_xfr_typ |= CMD_XFR_TYP_CCCEN;
		break;
	case MMC_RESPONSE_R1B:
		cmd_xfr_typ |= CMD_XFR_TYP_RSPTYP_48_BUSY;
		cmd_xfr_typ |= CMD_XFR_TYP_CICEN;
		cmd_xfr_typ |= CMD_XFR_TYP_CCCEN;
		break;
	default:
		break;
	}

	adtc_mask = ADTC_MASK_FROM_CMD_XFR_TYP(mmio_read_32(USDHC_CMD_XFR_TYP));
	if (adtc_mask & (BIT(cmd->cmd_idx))) {
		cmd_xfr_typ |= CMD_XFR_TYP_DPSEL;
		mix_ctrl |= MIX_CTRL_DTDSEL;
		mix_ctrl |= MIX_CTRL_DMAEN;
	}
	if (cmd->cmd_idx == MMC_CMD(18))
		mix_ctrl |= MIX_CTRL_BCEN;

	if (cmd->cmd_idx != MMC_CMD(55) && prepare_ds_addr) {
		if (BLKCNT_FROM_BLK_ATT(prepare_blk_att) > 1)
			mix_ctrl |= MIX_CTRL_BCEN | MIX_CTRL_MSBSEL;
		mmio_write_32(USDHC_MIX_CTRL, mix_ctrl);
		mmio_write_32(USDHC_DS_ADDR, prepare_ds_addr);
		mmio_write_32(USDHC_BLK_ATT, prepare_blk_att);
		prepare_ds_addr = 0;
	} else {
		mmio_write_32(USDHC_MIX_CTRL, mix_ctrl);
	}

	mmio_write_32(USDHC_CMDARG, cmd->cmd_arg);
	mmio_write_32(USDHC_CMD_XFR_TYP, cmd_xfr_typ);

	/* Without a delay here, the values read from USDHC_CMD_RSP(x)
	 * may not be up-to-date.
	 */
	udelay(1000);

	while ((regdata = mmio_read_32(USDHC_INT_STATUS))) {
		if (regdata & INT_STATUS_CMD_ERROR)
			goto cmd_error;
		if (regdata & INT_STATUS_CC)
			break;
	}

	if (cmd->resp_type & MMC_RSP_136) {
		for (i = 0; i < 4; i++)
			cmd_rsp[i] = mmio_read_32(USDHC_CMD_RSP(i));
		cmd->resp_data[3] = (cmd_rsp[3] << 8) | (cmd_rsp[2] >> 24);
		cmd->resp_data[2] = (cmd_rsp[2] << 8) | (cmd_rsp[1] >> 24);
		cmd->resp_data[1] = (cmd_rsp[1] << 8) | (cmd_rsp[0] >> 24);
		cmd->resp_data[0] = (cmd_rsp[0] << 8);
	} else {
		cmd->resp_data[0] = mmio_read_32(USDHC_CMD_RSP(0));
	}

	if (adtc_mask & (BIT(cmd->cmd_idx))) {
		while ((regdata = mmio_read_32(USDHC_INT_STATUS))) {
			if (regdata & (INT_STATUS_DATA_ERROR))
				goto data_error;
			if (regdata & (INT_STATUS_TC | INT_STATUS_DINT))
				break;
		}
	}

	return 0;

data_error:
	mmio_write_32(USDHC_SYS_CTRL, SYS_CTRL_RSTD);
	while (mmio_read_32(USDHC_SYS_CTRL) & SYS_CTRL_RSTD)
		;
cmd_error:
	mmio_write_32(USDHC_SYS_CTRL, SYS_CTRL_RSTC);
	while (mmio_read_32(USDHC_SYS_CTRL) & SYS_CTRL_RSTC)
		;
	return -EIO;
}

static int s32g274a_mmc_set_ios(unsigned int clk, unsigned int width)
{
	uint32_t regdata;

	s32g274a_mmc_set_clk(clk);

	regdata = mmio_read_32(USDHC_PROT_CTRL);
	regdata &= ~(PROT_CTRL_DTW_MASK);
	if (width == MMC_BUS_WIDTH_4)
		regdata |= PROT_CTRL_DTW_4;
	else if (width == MMC_BUS_WIDTH_8)
		regdata |= PROT_CTRL_DTW_8;
	mmio_write_32(USDHC_PROT_CTRL, regdata);

	return 0;
}

/* Normally we could simply write DS_ADDR and BLK_ATT here, but DS_ADDR gets
 * reset whenever a command is executed. And sometimes the sequence of
 * calls from the tf-a core is prepare() --> send_cmd(MMC_CMD(55)) -->
 * send_cmd(MMC_ACMD(x)). In this case DS_ADDR is reset by the time MMC_ACMD(x)
 * is executed. And since setting BLK_ATT requires certain bits to be set in
 * MIX_CTRL, and MIX_CTRL is configured in send_cmd(), the simplest solution
 * is to save the desired values for DS_ADDR and BLK_ATT and apply them right
 * before executing the command that needs them to be set.
 */

static int s32g274a_mmc_prepare(int lba, uintptr_t buf, size_t size)
{
	uint32_t block_size;

	if (size <= MMC_BLOCK_SIZE)
		block_size = size;
	else
		block_size = MMC_BLOCK_SIZE;

	prepare_ds_addr = buf;
	prepare_blk_att = BLK_ATT_BLKCNT(size / block_size) |
			  BLK_ATT_BLKSIZE(block_size);

	return 0;
}

static int s32g274a_mmc_read(int lba, uintptr_t buf, size_t size)
{
	return 0;
}

static int s32g274a_mmc_write(int lba, uintptr_t buf, size_t size)
{
	return 0;
}

static const struct mmc_ops s32g274a_mmc_ops = {
	.init		= s32g274a_mmc_init,
	.send_cmd	= s32g274a_mmc_send_cmd,
	.set_ios	= s32g274a_mmc_set_ios,
	.prepare	= s32g274a_mmc_prepare,
	.read		= s32g274a_mmc_read,
	.write		= s32g274a_mmc_write,
};

int s32g274a_mmc_register(void)
{
	return mmc_init(&s32g274a_mmc_ops, MMC_FULL_SPEED_MODE_FREQUENCY,
			MMC_BUS_WIDTH_4, 0, &device_info);
}
