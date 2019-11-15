/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/mmio.h>
#include <assert.h>
#include "s32g_pinctrl.h"


static void linflex_config_pinctrl(int lf)
{
	assert(lf == 0);

	/* set PC09 - MSCR[41] - for UART0 TXD */
	mmio_write_32(SIUL2_0_MSCRn(SIUL2_PC09_MSCR_S32_G1_UART0),
		      SIUL2_MSCR_S32G_G1_PORT_CTRL_UART0_TXD);
	/* set PC10 - MSCR[42] - for UART0 RXD */
	mmio_write_32(SIUL2_0_MSCRn(SIUL2_PC10_MSCR_S32_G1_UART0),
		      SIUL2_MSCR_S32G_G1_PORT_CTRL_UART_RXD);
	/* set PC10 - MSCR[512]/IMCR[0] - for UART0 RXD */
	mmio_write_32(SIUL2_0_IMCRn(SIUL2_PC10_IMCR_S32_G1_UART0),
		      SIUL2_IMCR_S32G_G1_UART0_RXD_to_pad);
}

void s32g_plat_config_pinctrl(void)
{
	linflex_config_pinctrl(S32G_LINFLEX_MODULE);
}
