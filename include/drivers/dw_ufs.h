/*
 * Copyright (c) 2017-2018, ARM Limited and Contributors. All rights reserved.
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DW_UFS_H
#define DW_UFS_H

#include <stdint.h>
#include <drivers/ufs.h>

/* Bus Throtting */
#define BUSTHRTL				0xC0
/* Outstanding OCP Requests */
#define OOCPR					0xC4
/* Fatal Error Interrupt Enable */
#define FEIE					0xC8
/* C-Port Direct Access Configuration register */
#define CDACFG					0xD0
/* C-Port Direct Access Transmit 1 register */
#define CDATX1					0xD4
/* C-Port Direct Access Transmit 2 register */
#define CDATX2					0xD8
/* C-Port Direct Access Receive 1 register */
#define CDARX1					0xDC
/* C-Port Direct Access Receive 2 register */
#define CDARX2					0xE0
/* C-Port Direct Access Status register */
#define CDASTA					0xE4
/* UPIU Loopback Configuration register */
#define LBMCFG					0xF0
/* UPIU Loopback Status */
#define LBMSTA					0xF4
/* Debug register */
#define DBG					0xF8
/* HClk Divider register */
#define HCLKDIV					0xFC

#define TX_HIBERN8TIME_CAP_OFFSET		0x000F
#define TX_FSM_STATE_OFFSET			0x0041
#define TX_FSM_STATE_LINE_RESET			7
#define TX_FSM_STATE_LINE_CFG			6
#define TX_FSM_STATE_HS_BURST			5
#define TX_FSM_STATE_LS_BURST			4
#define TX_FSM_STATE_STALL			3
#define TX_FSM_STATE_SLEEP			2
#define TX_FSM_STATE_HIBERN8			1
#define TX_FSM_STATE_DISABLE			0

#define TX_HS_UNTERMINATED_LINE_DRIVE_CAP_OFFSET        0x0007
#define TX_LS_TERMINATED_LINE_DRIVE_CAP_OFFSET          0x0008
#define RX_HS_UNTERMINATED_CAP_OFFSET                   0x0085
#define RX_LS_TERMINATED_CAP_OFFSET                     0x0086
#define RX_MIN_ACTIVATETIME_CAP_OFFSET			0x008F
#define RX_HS_G2_SYNC_LENGTH_CAP_OFFSET			0x0094
#define RX_HS_G3_SYNC_LENGTH_CAP_OFFSET			0x0095
#define RX_HS_ADAPT_REFRESH_CAP_OFFSET			0x009E
#define RX_HS_ADAPT_INITIAL_CAP_OFFSET			0x009F

#define PA_AVAIL_TX_DATA_LANES_OFFSET		0x1520
#define PA_TX_SKIP_OFFSET			0x155C
#define PA_TX_SKIP_PERIOD_OFFSET		0x155D
#define PA_LOCAL_TX_LCC_ENABLE_OFFSET		0x155E
#define PA_ACTIVE_TX_DATA_LANES_OFFSET		0x1560
#define PA_CONNECTED_TX_DATA_LANES_OFFSET	0x1561
#define PA_TX_TRAILING_CLOCKS_OFFSET		0x1564
#define PA_TX_GEAR_OFFSET			0x1568
#define PA_TX_TERMINATION_OFFSET		0x1569
#define PA_HS_SERIES_OFFSET			0x156A
#define PA_PWR_MODE_OFFSET			0x1571
#define PA_ACTIVE_RX_DATA_LANES_OFFSET		0x1580
#define PA_CONNECTED_RX_DATA_LANES_OFFSET	0x1581
#define PA_RX_PWR_STATUS_OFFSET			0x1582
#define PA_RX_GEAR_OFFSET			0x1583
#define PA_RX_TERMINATION_OFFSET		0x1584
#define PA_SCRAMBLING_OFFSET			0x1585
#define PA_MAX_RX_PWM_GEAR_OFFSET		0x1586
#define PA_MAX_RX_HS_GEAR_OFFSET		0x1587
#define PA_PACP_REQ_TIMEOUT_OFFSET		0x1590
#define PA_PACP_REQ_EOB_TIMEOUT_OFFSET		0x1591
#define PA_REMOTE_VER_INFO_OFFSET		0x15A0
#define PA_LOGICAL_LANE_MAP_OFFSET		0x15A1
#define PA_TACTIVATE_OFFSET			0x15A8
#define PA_PWR_MODE_USER_DATA0_OFFSET		0x15B0
#define PA_PWR_MODE_USER_DATA1_OFFSET		0x15B1
#define PA_PWR_MODE_USER_DATA2_OFFSET		0x15B2
#define PA_PWR_MODE_USER_DATA3_OFFSET		0x15B3
#define PA_PWR_MODE_USER_DATA4_OFFSET		0x15B4
#define PA_PWR_MODE_USER_DATA5_OFFSET		0x15B5

#define DL_TC0_TX_FC_THRESHOLD_OFFSET		0x2040
#define DL_AFC0_CREDIT_THRESHOLD_OFFSET		0x2044
#define DL_TC0_OUT_ACK_THRESHOLD_OFFSET		0x2045

#define N_DEVICEID_OFFSET			0x3000

#define T_CONNECTIONSTATE_OFFSET		0x4020
#define T_CPORTFLAGS_OFFSET			0x4025

#define RXSQCONTROL_OFFSET			0x8009
#define RXOVRCTRL1_OFFSET			0x800A
#define RXC10DIRECTCONF7_OFFSET			0x8010
#define RXLINERSTCFGMSB_OFFSET			0x8011
#define RXLINERSTCFGLSB_OFFSET			0x8012
#define RXRHOLDCTRLOPT_OFFSET			0x8013
#define TX_EQ_MAIN_DE00_LA_OFFSET		0x8014
#define TX_EQ_MAIN_DE03_LA_OFFSET		0x801A
#define TX_EQ_MAIN_DE06_SA_OFFSET		0x801D
#define TX_EQ_POST_DE06_SA_OFFSET		0x801E
#define TX_EQ_MAIN_DE06_LA_OFFSET		0x8020
#define TX_EQ_POST_DE06_LA_OFFSET		0x8021
#define TXC10_IBOOST_RBOOST_CONF_OFFSET		0x8024
#define CBRATESEL_OFFSET			0x8114
#define CBCREGRDLSB_OFFSET			0x811A
#define CBCREGRDMSB_OFFSET			0x811B
#define CBENBLCPBATTRWR_OFFSET			0x8113
#define CBCREGADDRLSB_OFFSET			0x8116
#define CBCREGADDRMSB_OFFSET			0x8117
#define CBCREGWRLSB_OFFSET			0x8118
#define CBCREGWRMSB_OFFSET			0x8119
#define CBCREGRDWRSEL_OFFSET			0x811C
#define CBCRCTRL_OFFSET				0x811F
#define CBUPLH8_OFFSET				0x8130
#define CBREFCLKCTRL2_OFFSET			0x8132
#define MPLLBFRACNDENLSB_OFFSET			0x8141
#define V2IDIV5DIV2MPLLB_OFFSET			0x8160

#define DME_FC0_PROTECTION_TIMEOUT_OFFSET	0xD041
#define DME_TC0_REPLAY_TIMEOUT_OFFSET		0xD042
#define DME_AFC0_REQ_TIMEOUT_OFFSET		0xD043
#define DME_FC1_PROTECTION_TIMEOUT_OFFSET	0xD044
#define DME_TC1_REPLAY_TIMEOUT_OFFSET		0xD045
#define DME_AFC1_REQ_TIMEOUT_OFFSET		0xD046

#define VS_MPHY_CFG_UPDT_OFFSET			0xD085
#define VS_MK2_EXTN_SUPPORT_OFFSET		0xD0AB
#define VS_MPHY_DISABLE_OFFSET			0xD0C1
#define VS_MPHY_DISABLE_MPHYDIS			(1 << 0)

#define SEL_TX_LANE0				0x0
#define SEL_TX_LANE1				0x1
#define SEL_RX_LANE0				0x4
#define SEL_RX_LANE1				0x5

/* CPort setting */
#define CPORT_0					0
#define CPORT_1					1

#define E2EFC_ON				(1 << 0)
#define E2EFC_OFF				(0 << 0)
#define CSD_N_ON				(0 << 1)
#define CSD_N_OFF				(1 << 1)
#define CSV_N_ON				(0 << 2)
#define CSV_N_OFF				(1 << 2)
#define CPORT_FLAGS(csv_n, csd_n, e2e)		((csv_n) | (csd_n) | (e2e))

/* PA TX/RX Frequency Series */
enum {
	PA_HS_MODE_A	= 1,
	PA_HS_MODE_B	= 2,
};

/* CPort connection state */
enum {
	CPORT_IDLE	= 0,
	CPORT_CONNECTED	= 1,
};

typedef struct dw_ufs_params {
	uintptr_t		reg_base;
	uintptr_t		desc_base;
	size_t			desc_size;
	unsigned long		flags;
} dw_ufs_params_t;

int dwufs_phy_set_pwr_mode(ufs_params_t *params);
int dw_ufs_init(dw_ufs_params_t *params);

#endif /* DW_UFS_H */
