/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <platform_def.h>

#include <s32_bl_common.h>
#include <drivers/delay_timer.h>
#include <drivers/mmc.h>
#include <lib/utils.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

#define USDHC_DS_ADDR			(USDHC_BASE_ADDR + 0x0)
#define USDHC_BLK_ATT			(USDHC_BASE_ADDR + 0x4)
#define BLK_ATT_BLKCNT(x)		(((x) & 0xffff) << 16)
#define BLKCNT_FROM_BLK_ATT(r)		(((r) >> 16) & 0xffff)
#define BLK_ATT_BLKSIZE(x)		((x) & 0x1fff)

#define USDHC_CMDARG			(USDHC_BASE_ADDR + 0x8)
#define USDHC_CMD_XFR_TYP		(USDHC_BASE_ADDR + 0xc)
#define CMD_XFR_TYP_RSPTYP(x)		(((x) & 0x3) << 16)
#define CMD_XFR_TYP_RSPTYP_136		CMD_XFR_TYP_RSPTYP(0x1)
#define CMD_XFR_TYP_RSPTYP_48		CMD_XFR_TYP_RSPTYP(0x2)
#define CMD_XFR_TYP_RSPTYP_48_BUSY	CMD_XFR_TYP_RSPTYP(0x3)
#define CMD_XFR_TYP_DPSEL		BIT(21)
#define CMD_XFR_TYP_CICEN		BIT(20)
#define CMD_XFR_TYP_CCCEN		BIT(19)
#define CMD_XFR_TYP_CMDINX(x)		(((x) & 0x3f) << 24)
#define CMDINX_FROM_CMD_XFR_TYP(r)	(((r) >> 24) & 0x3f)
#define USDHC_CMD_RSP(i)		(USDHC_BASE_ADDR + 0x10 + (i) * 0x4)

#define USDHC_PRES_STATE		(USDHC_BASE_ADDR + 0x24)
#define PRES_STATE_CIHB			BIT(0)
#define PRES_STATE_CDIHB		BIT(1)
#define PRES_STATE_DLA			BIT(2)
#define PRES_STATE_SDSTB		BIT(3)

#define USDHC_PROT_CTRL			(USDHC_BASE_ADDR + 0x28)
#define PROT_CTRL_EMODE_LE		BIT(5)
#define PROT_CTRL_DTW_4			BIT(1)
#define PROT_CTRL_DTW_8			BIT(2)
#define PROT_CTRL_DTW_MASK		(0x6)

#define USDHC_SYS_CTRL			(USDHC_BASE_ADDR + 0x2c)
#define SYS_CTRL_RSTD			BIT(26)
#define SYS_CTRL_RSTC			BIT(25)
#define SYS_CTRL_RSTA			BIT(24)
#define SYS_CTRL_DTOCV(x)		(((x) & 0xf) << 16)
#define SYS_CTRL_DTOCV_MASK		(SYS_CTRL_DTOCV(0xf))
#define SYS_CTRL_SDCLKFS(x)		(((x) & 0xff) << 8)
#define SYS_CTRL_SDCLKFS_MASK		(SYS_CTRL_SDCLKFS(0xff))
#define SYS_CTRL_DVS(x)			(((x) & 0xf) << 4)
#define SYS_CTRL_DVS_MASK		(SYS_CTRL_DVS(0xf))

#define USDHC_INT_STATUS		(USDHC_BASE_ADDR + 0x30)
#define INT_STATUS_DMAE			BIT(28)
#define INT_STATUS_DEBE			BIT(22)
#define INT_STATUS_DCE			BIT(21)
#define INT_STATUS_DTOE			BIT(20)
#define INT_STATUS_CIE			BIT(19)
#define INT_STATUS_CEBE			BIT(18)
#define INT_STATUS_CCE			BIT(17)
#define INT_STATUS_CTOE			BIT(16)
#define INT_STATUS_TC			BIT(1)
#define INT_STATUS_CC			BIT(0)
#define INT_STATUS_CMD_ERROR		(INT_STATUS_CIE | INT_STATUS_CEBE | \
					 INT_STATUS_CCE | INT_STATUS_CTOE)
#define INT_STATUS_DATA_ERROR		(INT_STATUS_DMAE | INT_STATUS_DEBE | \
					 INT_STATUS_DCE | INT_STATUS_DTOE)
#define INT_STATUS_CLEARMASK		(INT_STATUS_CC | INT_STATUS_TC | \
					 INT_STATUS_CMD_ERROR | \
					 INT_STATUS_DATA_ERROR)

#define USDHC_INT_STATUS_EN		(USDHC_BASE_ADDR + 0x34)
#define INT_STATUS_EN_ENABLEMASK	INT_STATUS_CLEARMASK

#define USDHC_INT_SIGNAL_EN		(USDHC_BASE_ADDR + 0x38)

#define USDHC_WTMK_LVL			(USDHC_BASE_ADDR + 0x44)
#define WTMK_LVL_WR_WML_SHIFT	(16)
#define WTMK_LVL_WR_WML_DEF_VAL	(64)
#define WTMK_LVL_WR_WML(x)		(((x) & 0xff) << WTMK_LVL_WR_WML_SHIFT)

#define USDHC_MIX_CTRL			(USDHC_BASE_ADDR + 0x48)
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

#define USDHC_DLL_CTRL			(USDHC_BASE_ADDR + 0x60)
#define USDHC_CLK_TUNE_CTRL_STATUS	(USDHC_BASE_ADDR + 0x68)
#define USDHC_MMC_BOOT			(USDHC_BASE_ADDR + 0xc4)

#define USDHC_VEND_SPEC			(USDHC_BASE_ADDR + 0xc0)
#define VEND_SPEC_INIT			(0x20007809)

/* These masks represent the commands which involve a data transfer. */
#define ADTC_MASK_SD			(BIT(6) | BIT(18) | BIT(17) | BIT(24) | BIT(25))
#define ADTC_MASK_MMC			(BIT(8) | BIT(18) | BIT(17) | BIT(24) | BIT(25))
#define ADTC_MASK_ACMD			(BIT(51))

#define IDENTIFICATION_MODE_FREQUENCY	(400 * 1000)
#define SD_FULL_SPEED_MODE_FREQUENCY	(25 * 1000 * 1000)
#define MMC_FULL_SPEED_MODE_FREQUENCY	(26 * 1000 * 1000)

static struct mmc_device_info emmc_device_info = {
	.mmc_dev_type = MMC_IS_EMMC,
	.op_cond_period = 1,
};

static struct mmc_device_info sd_device_info = {
	.mmc_dev_type = MMC_IS_SD_HC,
	.ocr_voltage = OCR_3_2_3_3 | OCR_3_3_3_4,
	.op_cond_period = 1,
};

struct s32_usdhc_device_data {
	struct mmc_device_info *devinfo;
	uint32_t prepare_ds_addr;
	uint32_t prepare_blk_att;
};

static struct s32_usdhc_device_data devdata;

static bool is_data_transfer_to_host(unsigned int cmd_idx)
{
	return cmd_idx == MMC_CMD(24) || cmd_idx == MMC_CMD(25);
}

static bool is_multiple_block_transfer(unsigned int cmd_idx)
{
	return cmd_idx == MMC_CMD(18) || cmd_idx == MMC_CMD(25);
}

static bool is_data_transfer_command(uint8_t opcode)
{
	uint32_t cmd_xfr_typ = mmio_read_32(USDHC_CMD_XFR_TYP);

	if (CMDINX_FROM_CMD_XFR_TYP(cmd_xfr_typ) == MMC_CMD(55))
		return ADTC_MASK_ACMD & BIT(opcode);

	if (devdata.devinfo == &emmc_device_info)
		return ADTC_MASK_MMC & BIT(opcode);

	return ADTC_MASK_SD & BIT(opcode);
}

static void s32_mmc_set_clk(uint64_t clk)
{
	uint32_t regdata;
	int prediv = 1;
	int div = 1;
	unsigned long sdhc_clk_freq = get_sdhc_clk_freq();

	assert(!(mmio_read_32(USDHC_MIX_CTRL) & MIX_CTRL_DDR_EN));

	while (sdhc_clk_freq / (prediv * 16) > clk && prediv < 256)
		prediv <<= 1;
	while (sdhc_clk_freq / (prediv * div) > clk && div < 16)
		div++;
	prediv >>= 1;
	div--;

	while (!(mmio_read_32(USDHC_PRES_STATE) & PRES_STATE_SDSTB))
		;

	regdata = mmio_read_32(USDHC_SYS_CTRL);
	regdata &= ~(SYS_CTRL_SDCLKFS_MASK | SYS_CTRL_DVS_MASK);
	regdata |= SYS_CTRL_SDCLKFS(prediv) | SYS_CTRL_DVS(div);
	mmio_write_32(USDHC_SYS_CTRL, regdata);

	while (!(mmio_read_32(USDHC_PRES_STATE) & PRES_STATE_SDSTB))
		;
}

static void s32_mmc_init(void)
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

	s32_mmc_set_clk(IDENTIFICATION_MODE_FREQUENCY);

	mmio_write_32(USDHC_INT_STATUS_EN, INT_STATUS_EN_ENABLEMASK);
	mmio_write_32(USDHC_INT_SIGNAL_EN, 0);
	mmio_write_32(USDHC_PROT_CTRL, PROT_CTRL_EMODE_LE);

	regdata = mmio_read_32(USDHC_SYS_CTRL);
	regdata &= ~SYS_CTRL_DTOCV_MASK;
	regdata |= SYS_CTRL_DTOCV(0xd);
	mmio_write_32(USDHC_SYS_CTRL, regdata);

	/* Set watermark level for write operation */
	regdata = mmio_read_32(USDHC_WTMK_LVL);
	regdata |= WTMK_LVL_WR_WML(WTMK_LVL_WR_WML_DEF_VAL);
	mmio_write_32(USDHC_WTMK_LVL, regdata);
}

static int s32_mmc_send_cmd(struct mmc_cmd *cmd)
{
	int i;
	uint32_t cmd_xfr_typ = 0;
	uint32_t mix_ctrl = 0;
	uint32_t cmd_rsp[4];
	uint32_t regdata;
	bool data_xfer = is_data_transfer_command(cmd->cmd_idx);

	mmio_write_32(USDHC_INT_STATUS, INT_STATUS_CLEARMASK);
	while (mmio_read_32(USDHC_PRES_STATE) &
	       (PRES_STATE_CDIHB | PRES_STATE_CIHB | PRES_STATE_DLA))
		;

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

	if (data_xfer) {
		cmd_xfr_typ |= CMD_XFR_TYP_DPSEL;
		mix_ctrl |= MIX_CTRL_DMAEN;
	}

	if (!is_data_transfer_to_host(cmd->cmd_idx))
		mix_ctrl |= MIX_CTRL_DTDSEL;

	if (is_multiple_block_transfer(cmd->cmd_idx))
		mix_ctrl |= MIX_CTRL_BCEN;

	if (cmd->cmd_idx != MMC_CMD(55) && devdata.prepare_ds_addr) {
		if (BLKCNT_FROM_BLK_ATT(devdata.prepare_blk_att) > 1)
			mix_ctrl |= MIX_CTRL_BCEN | MIX_CTRL_MSBSEL;
		mmio_write_32(USDHC_MIX_CTRL, mix_ctrl);
		mmio_write_32(USDHC_DS_ADDR, devdata.prepare_ds_addr);
		mmio_write_32(USDHC_BLK_ATT, devdata.prepare_blk_att);
		devdata.prepare_ds_addr = 0;
	} else {
		mmio_write_32(USDHC_MIX_CTRL, mix_ctrl);
	}

	mmio_write_32(USDHC_CMDARG, cmd->cmd_arg);
	mmio_write_32(USDHC_CMD_XFR_TYP, cmd_xfr_typ);

	do {
		regdata = mmio_read_32(USDHC_INT_STATUS);
		if (regdata & INT_STATUS_CMD_ERROR)
			goto cmd_error;
	} while (!(regdata & INT_STATUS_CC));

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

	if (data_xfer)
		do {
			regdata = mmio_read_32(USDHC_INT_STATUS);
			if (regdata & (INT_STATUS_DATA_ERROR))
				goto data_error;
		} while (!(regdata & INT_STATUS_TC));

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

static int s32_mmc_set_ios(unsigned int clk, unsigned int width)
{
	uint32_t regdata;

	s32_mmc_set_clk(clk);

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

static int s32_mmc_prepare(int lba, uintptr_t buf, size_t size)
{
	uint32_t block_size;

	if (size <= MMC_BLOCK_SIZE)
		block_size = size;
	else
		block_size = MMC_BLOCK_SIZE;

	devdata.prepare_ds_addr = buf;
	devdata.prepare_blk_att = BLK_ATT_BLKCNT(size / block_size) |
				  BLK_ATT_BLKSIZE(block_size);

	flush_dcache_range(buf, size);
	return 0;
}

static int s32_mmc_read(int lba, uintptr_t buf, size_t size)
{
	inv_dcache_range(buf, size);
	return 0;
}

static int s32_mmc_write(int lba, uintptr_t buf, size_t size)
{
	return 0;
}

static const struct mmc_ops s32_mmc_ops = {
	.init		= s32_mmc_init,
	.send_cmd	= s32_mmc_send_cmd,
	.set_ios	= s32_mmc_set_ios,
	.prepare	= s32_mmc_prepare,
	.read		= s32_mmc_read,
	.write		= s32_mmc_write,
};

static bool s32_is_card_emmc(void)
{
	struct mmc_cmd cmd;

	zeromem(&cmd, sizeof(struct mmc_cmd));
	cmd.cmd_idx = MMC_CMD(0);
	cmd.cmd_arg = 0;
	cmd.resp_type = 0;

	if (s32_mmc_send_cmd(&cmd)) {
		ERROR("%s: Failed to reset card to idle state!\n", __func__);
		panic();
	}

	/* MMC_CMD(1) is SEND_OP_COND for eMMC, but reserved for SD.
	 * If the command succeeds, the underlying card must be an eMMC.
	 */
	zeromem(&cmd, sizeof(struct mmc_cmd));
	cmd.cmd_idx = MMC_CMD(1);
	cmd.cmd_arg = OCR_SECTOR_MODE | OCR_VDD_MIN_2V7 | OCR_VDD_MIN_1V7;
	cmd.resp_type = MMC_RESPONSE_R3;

	return (!s32_mmc_send_cmd(&cmd));
}

int s32_mmc_register(void)
{
	unsigned int clk, bus_width;
	int result;

	result = mmap_add_dynamic_region(USDHC_BASE_ADDR, USDHC_BASE_ADDR,
				      MMU_ROUND_UP_TO_PAGE(USDHC_SIZE),
				      MT_DEVICE | MT_RW);
	if (result < 0) {
		ERROR("uSDHC mapping: error %d\n", result);
		panic();
	}

	s32_mmc_init();

	devdata.prepare_ds_addr = 0;
	devdata.prepare_blk_att = 0;

	if (s32_is_card_emmc()) {
		devdata.devinfo = &emmc_device_info;
		bus_width = MMC_BUS_WIDTH_8;
		clk = MMC_FULL_SPEED_MODE_FREQUENCY;
		return mmc_init(&s32_mmc_ops, clk, bus_width,
				0, devdata.devinfo);
	}

	devdata.devinfo = &sd_device_info;
	bus_width = MMC_BUS_WIDTH_4;
	clk = SD_FULL_SPEED_MODE_FREQUENCY;

	return mmc_init(&s32_mmc_ops, clk, bus_width,
			MMC_FLAG_SD_CMD6, devdata.devinfo);
}
