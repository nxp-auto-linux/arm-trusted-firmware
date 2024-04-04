/*
 * Copyright 2019-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _S32G_PINCTRL_H_
#define _S32G_PINCTRL_H_

#include <lib/mmio.h>
#include "platform_def.h"
#include "s32cc_pinctrl.h"

/*
 * Pinctrl for I2C
 */
#define SIUL2_MSCR_S32G_PB_00   (16)
#define SIUL2_MSCR_S32G_PB_01   (17)
#define SIUL2_MSCR_S32G_PB_03   (19)
#define SIUL2_MSCR_S32G_PB_04   (20)
#define SIUL2_MSCR_S32G_PB_05   (21)
#define SIUL2_MSCR_S32G_PB_06   (22)
#define SIUL2_MSCR_S32G_PB_07   (23)
#define SIUL2_MSCR_S32G_PB_13   (29)
#define SIUL2_MSCR_S32G_PB_15   (31)
#define SIUL2_MSCR_S32G_PC_00   (32)
#define SIUL2_MSCR_S32G_PC_01   (33)
#define SIUL2_MSCR_S32G_PC_02   (34)
#define SIUL2_MSCR_S32G_PK_03   (163)
#define SIUL2_MSCR_S32G_PK_05   (165)
#define SIUL2_PB_00_IMCR_S32G_I2C0_SDA  (565)
#define SIUL2_PB_01_IMCR_S32G_I2C0_SCLK (566)
#define SIUL2_PB_03_IMCR_S32G_I2C1_SCLK (717)
#define SIUL2_PB_04_IMCR_S32G_I2C1_SDA  (718)
#define SIUL2_PK_03_IMCR_S32G_I2C1_SCLK (717)
#define SIUL2_PK_05_IMCR_S32G_I2C1_SDA  (718)
#define SIUL2_PB_05_IMCR_S32G_I2C2_SCLK (719)
#define SIUL2_PB_06_IMCR_S32G_I2C2_SDA  (720)
#define SIUL2_PB_07_IMCR_S32G_I2C3_SCLK (721)
#define SIUL2_PB_13_IMCR_S32G_I2C3_SDA  (722)
#define SIUL2_PC_01_IMCR_S32G_I2C4_SDA  (724)
#define SIUL2_PC_02_IMCR_S32G_I2C4_SCLK (723)
#define SIUL2_PB_15_IMCR_S32G_I2C1_SDA  (565)
#define SIUL2_PC_00_IMCR_S32G_I2C1_SCLK (566)

/* SIUL2_MIDR1 masks */
#define SIUL2_MIDR1_MINOR_MASK		(0xF << 0)
#define SIUL2_MIDR1_MAJOR_SHIFT		(4)
#define SIUL2_MIDR1_MAJOR_MASK		(0xF << SIUL2_MIDR1_MAJOR_SHIFT)

void i2c_config_pinctrl(void);
void wkpu_config_pinctrl(uint32_t wkup_irq);

#endif
