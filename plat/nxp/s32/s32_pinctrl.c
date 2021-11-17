/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/mmio.h>
#include <assert.h>
#include "s32_pinctrl.h"

static void linflex_config_pinctrl(int lf)
{
	if (!lf) {
		/* set PC09 - MSCR[41] - for UART0 TXD */
		mmio_write_32(SIUL2_0_MSCRn(SIUL2_PC09_MSCR_S32_G1_UART0),
			      SIUL2_MSCR_S32G_G1_PORT_CTRL_UART0_TXD);
		/* set PC10 - MSCR[42] - for UART0 RXD */
		mmio_write_32(SIUL2_0_MSCRn(SIUL2_PC10_MSCR_S32_G1_UART0),
			      SIUL2_MSCR_S32G_G1_PORT_CTRL_UART_RXD);
		/* set PC10 - MSCR[512]/IMCR[0] - for UART0 RXD */
		mmio_write_32(SIUL2_0_IMCRn(SIUL2_PC10_IMCR_S32_G1_UART0),
			      SIUL2_IMCR_S32G_G1_UART0_RXD_to_pad);
		return;
	}

	if (lf == 1) {
		/* set PB09 - MSCR[25] - for UART1 TXD */
		mmio_write_32(SIUL2_0_MSCRn(SIUL2_PB09_MSCR_S32_G1_UART1),
			      SIUL2_MSCR_S32G_G1_PORT_CTRL_UART1_TXD);

		/* set PB10 - MSCR[26] - for UART1 RXD */
		mmio_write_32(SIUL2_0_MSCRn(SIUL2_PB10_MSCR_S32_G1_UART1),
			      SIUL2_MSCR_S32G_G1_PORT_CTRL_UART_RXD);

		/* set PB10 - MSCR[736]/IMCR[224] - for UART1 RXD */
		mmio_write_32(SIUL2_1_IMCRn(SIUL2_PB10_IMCR_S32_G1_UART1),
			      SIUL2_IMCR_S32G_G1_UART1_RXD_to_pad);
	}
}

static void sdhc_config_pinctrl(void)
{
	/* Set iomux PADS for USDHC */

	/* PC14 pad: uSDHC SD0_CLK_O  */
	mmio_write_32(SIUL2_0_MSCRn(46), SIUL2_USDHC_S32_G1_PAD_CTRL_CLK);

	/* PC15 pad: uSDHC SDO_CMD_0 */
	mmio_write_32(SIUL2_0_MSCRn(47), SIUL2_USDHC_S32_G1_PAD_CTRL_CMD);
	mmio_write_32(SIUL2_0_MSCRn(515), 0x2);

	/* PD00 pad: uSDHC SD0_D_O[0] */
	mmio_write_32(SIUL2_0_MSCRn(48), SIUL2_USDHC_S32_G1_PAD_CTRL_DATA);
	mmio_write_32(SIUL2_0_MSCRn(516), 0x2);

	/* PD01 pad: uSDHC SD0_D_O[1] */
	mmio_write_32(SIUL2_0_MSCRn(49), SIUL2_USDHC_S32_G1_PAD_CTRL_DATA);
	mmio_write_32(SIUL2_0_MSCRn(517), 0x2);

	/* PD02 pad: uSDHC SD0_D_O[2] */
	mmio_write_32(SIUL2_0_MSCRn(50), SIUL2_USDHC_S32_G1_PAD_CTRL_DATA);
	mmio_write_32(SIUL2_0_MSCRn(520), 0x2);

	/* PD03 pad: uSDHC SD0_D_O[3] */
	mmio_write_32(SIUL2_0_MSCRn(51), SIUL2_USDHC_S32_G1_PAD_CTRL_DATA);
	mmio_write_32(SIUL2_0_MSCRn(521), 0x2);

	/* PD04 pad: uSDHC SD0_D_O[4] */
	mmio_write_32(SIUL2_0_MSCRn(52), SIUL2_USDHC_S32_G1_PAD_CTRL_DATA);
	mmio_write_32(SIUL2_0_MSCRn(522), 0x2);

	/* PD05 pad: uSDHC SD0_D_O[5] */
	mmio_write_32(SIUL2_0_MSCRn(53), SIUL2_USDHC_S32_G1_PAD_CTRL_DATA);
	mmio_write_32(SIUL2_0_MSCRn(523), 0x2);

	/* PD06 pad: uSDHC SD0_D_O[6] */
	mmio_write_32(SIUL2_0_MSCRn(54), SIUL2_USDHC_S32_G1_PAD_CTRL_DATA);
	mmio_write_32(SIUL2_0_MSCRn(519), 0x2);

	/* PD07 pad: uSDHC SD0_D_O[7] */
	mmio_write_32(SIUL2_0_MSCRn(55), SIUL2_USDHC_S32_G1_PAD_CTRL_DATA);
	mmio_write_32(SIUL2_0_MSCRn(518), 0x2);

	/* PD08 pad: uSDHC SDO_RST */
	mmio_write_32(SIUL2_0_MSCRn(56), SIUL2_USDHC_S32_G1_PAD_RST);

	/* PD10 pad: uSDHC SD0_DQS_I */
	mmio_write_32(SIUL2_0_MSCRn(524), 0x2);
}

void s32_plat_config_pinctrl(void)
{
	linflex_config_pinctrl(S32_LINFLEX_MODULE);
	sdhc_config_pinctrl();
}
