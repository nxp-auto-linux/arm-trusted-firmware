// SPDX-License-Identifier: GPL-2.0+
/*
 * i2c driver for Freescale i.MX series
 *
 * (c) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * (c) 2011 Marek Vasut <marek.vasut@gmail.com>
 *
 * Based on i2c-imx.c from linux kernel:
 *  Copyright (C) 2005 Torsten Koschorrek <koschorrek at synertronixx.de>
 *  Copyright (C) 2005 Matthias Blaschke <blaschke at synertronixx.de>
 *  Copyright (C) 2007 RightHand Technologies, Inc.
 *  Copyright (C) 2008 Darius Augulis <darius.augulis at teltonika.lt>
 *  Copyright 2020 NXP
 *
 */

#include <stdio.h>
#include <lib/mmio.h>
#include <libfdt.h>
#include <drivers/delay_timer.h>
#include <drivers/nxp/s32g/i2c/s32g_i2c.h>
#include "s32g_clocks.h"
#include "s32g_dt.h"

#define I2C_QUIRK_FLAG		(1 << 0)
#define I2C_QUIRK_REG

#define IMX_I2C_REGSHIFT	2
#define VF610_I2C_REGSHIFT	0

/* Register index */
#define IADR	0
#define IFDR	1
#define I2CR	2
#define I2SR	3
#define I2DR	4

#define I2CR_IIEN	(1 << 6)
#define I2CR_MSTA	(1 << 5)
#define I2CR_MTX	(1 << 4)
#define I2CR_TX_NO_AK	(1 << 3)
#define I2CR_RSTA	(1 << 2)

#define I2SR_ICF	(1 << 7)
#define I2SR_IBB	(1 << 5)
#define I2SR_IAL	(1 << 4)
#define I2SR_IIF	(1 << 1)
#define I2SR_RX_NO_AK	(1 << 0)

#ifdef I2C_QUIRK_REG
#define I2CR_IEN	(0 << 7)
#define I2CR_IDIS	(1 << 7)
#define I2SR_IIF_CLEAR	(1 << 1)
#else
#define I2CR_IEN	(1 << 7)
#define I2CR_IDIS	(0 << 7)
#define I2SR_IIF_CLEAR	(0 << 1)
#endif

#ifdef I2C_QUIRK_REG
static uint16_t i2c_clk_div[60][2] = {
	{ 20,	0x00 }, { 22,	0x01 }, { 24,	0x02 }, { 26,	0x03 },
	{ 28,	0x04 },	{ 30,	0x05 },	{ 32,	0x09 }, { 34,	0x06 },
	{ 36,	0x0A }, { 40,	0x07 }, { 44,	0x0C }, { 48,	0x0D },
	{ 52,	0x43 },	{ 56,	0x0E }, { 60,	0x45 }, { 64,	0x12 },
	{ 68,	0x0F },	{ 72,	0x13 },	{ 80,	0x14 },	{ 88,	0x15 },
	{ 96,	0x19 },	{ 104,	0x16 },	{ 112,	0x1A },	{ 128,	0x17 },
	{ 136,	0x4F }, { 144,	0x1C },	{ 160,	0x1D }, { 176,	0x55 },
	{ 192,	0x1E }, { 208,	0x56 },	{ 224,	0x22 }, { 228,	0x24 },
	{ 240,	0x1F },	{ 256,	0x23 }, { 288,	0x5C },	{ 320,	0x25 },
	{ 384,	0x26 }, { 448,	0x2A },	{ 480,	0x27 }, { 512,	0x2B },
	{ 576,	0x2C },	{ 640,	0x2D },	{ 768,	0x31 }, { 896,	0x32 },
	{ 960,	0x2F },	{ 1024,	0x33 },	{ 1152,	0x34 }, { 1280,	0x35 },
	{ 1536,	0x36 }, { 1792,	0x3A },	{ 1920,	0x37 },	{ 2048,	0x3B },
	{ 2304,	0x3C },	{ 2560,	0x3D },	{ 3072,	0x3E }, { 3584,	0x7A },
	{ 3840,	0x3F }, { 4096,	0x7B }, { 5120,	0x7D },	{ 6144,	0x7E },
};
#else
static uint16_t i2c_clk_div[50][2] = {
	{ 22,	0x20 }, { 24,	0x21 }, { 26,	0x22 }, { 28,	0x23 },
	{ 30,	0x00 }, { 32,	0x24 }, { 36,	0x25 }, { 40,	0x26 },
	{ 42,	0x03 }, { 44,	0x27 }, { 48,	0x28 }, { 52,	0x05 },
	{ 56,	0x29 }, { 60,	0x06 }, { 64,	0x2A }, { 72,	0x2B },
	{ 80,	0x2C }, { 88,	0x09 }, { 96,	0x2D }, { 104,	0x0A },
	{ 112,	0x2E }, { 128,	0x2F }, { 144,	0x0C }, { 160,	0x30 },
	{ 192,	0x31 }, { 224,	0x32 }, { 240,	0x0F }, { 256,	0x33 },
	{ 288,	0x10 }, { 320,	0x34 }, { 384,	0x35 }, { 448,	0x36 },
	{ 480,	0x13 }, { 512,	0x37 }, { 576,	0x14 }, { 640,	0x38 },
	{ 768,	0x39 }, { 896,	0x3A }, { 960,	0x17 }, { 1024,	0x3B },
	{ 1152,	0x18 }, { 1280,	0x3C }, { 1536,	0x3D }, { 1792,	0x3E },
	{ 1920,	0x1B }, { 2048,	0x3F }, { 2304,	0x1C }, { 2560,	0x1D },
	{ 3072,	0x1E }, { 3840,	0x1F }
};
#endif

/*
 * Calculate and set proper clock divider
 */
static uint8_t i2c_imx_get_clk(struct s32g_i2c_bus *i2c_bus, unsigned int rate)
{
	unsigned int i2c_clk_rate;
	unsigned int div;
	uint8_t clk_div;

	/* Divider value calculation */
	i2c_clk_rate = I2C_CLK_FREQ;
	div = (i2c_clk_rate + rate - 1) / rate;
	if (div < i2c_clk_div[0][0])
		clk_div = 0;
	else if (div > i2c_clk_div[ARRAY_SIZE(i2c_clk_div) - 1][0])
		clk_div = ARRAY_SIZE(i2c_clk_div) - 1;
	else
		for (clk_div = 0; i2c_clk_div[clk_div][0] < div; clk_div++)
			;

	/* Store divider value */
	return clk_div;
}

/*
 * Set I2C Bus speed
 */
static int bus_i2c_set_bus_speed(struct s32g_i2c_bus *i2c_bus, int speed)
{
	unsigned long base = i2c_bus->base;
	bool quirk = i2c_bus->driver_data & I2C_QUIRK_FLAG ? true : false;
	uint8_t clk_idx = i2c_imx_get_clk(i2c_bus, speed);
	uint8_t idx = i2c_clk_div[clk_idx][1];
	int reg_shift = quirk ? VF610_I2C_REGSHIFT : IMX_I2C_REGSHIFT;

	if (!base)
		return -EINVAL;

	/* Store divider value */
	mmio_write_8(base + (IFDR << reg_shift), idx);

	/* Reset module */
	mmio_write_8(base + (I2CR << reg_shift), I2CR_IDIS);
	mmio_write_8(base + (I2SR << reg_shift), 0);
	return 0;
}

#define ST_BUS_IDLE (0 | (I2SR_IBB << 8))
#define ST_BUS_BUSY (I2SR_IBB | (I2SR_IBB << 8))
#define ST_IIF (I2SR_IIF | (I2SR_IIF << 8))

static int wait_for_sr_state(struct s32g_i2c_bus *i2c_bus, unsigned int state)
{
	unsigned int sr;
	bool quirk = i2c_bus->driver_data & I2C_QUIRK_FLAG ? true : false;
	int reg_shift = quirk ? VF610_I2C_REGSHIFT : IMX_I2C_REGSHIFT;
	unsigned long base = i2c_bus->base;
	uint32_t wait_cnt = 100000; /* .1 seconds */

	for (;;) {
		sr = mmio_read_8(base + (I2SR << reg_shift));
		if (sr & I2SR_IAL) {
			if (quirk)
				mmio_write_8(base + (I2SR << reg_shift),
						sr | I2SR_IAL);
			else
				mmio_write_8(base + (I2SR << reg_shift),
						sr & ~I2SR_IAL);
			INFO("%s: Arbitration lost sr=%x cr=%x state=%x\n",
					__func__, sr,
					mmio_read_8(base + (I2CR << reg_shift)),
					state);
			return -EINTR;
		}
		if ((sr & (state >> 8)) == (unsigned char)state)
			return sr;
		udelay(1);
		wait_cnt--;
		if (!wait_cnt)
			break;
	}
	INFO("%s: failed sr=%x cr=%x state=%x\n", __func__,
			sr, mmio_read_8(base + (I2CR << reg_shift)), state);
	return -ETIMEDOUT;
}

static int tx_byte(struct s32g_i2c_bus *i2c_bus, uint8_t byte)
{
	int ret;
	int reg_shift = i2c_bus->driver_data & I2C_QUIRK_FLAG ?
		VF610_I2C_REGSHIFT : IMX_I2C_REGSHIFT;
	unsigned long base = i2c_bus->base;

	mmio_write_8(base + (I2SR << reg_shift), I2SR_IIF_CLEAR);
	mmio_write_8(base + (I2DR << reg_shift), byte);

	ret = wait_for_sr_state(i2c_bus, ST_IIF);
	if (ret < 0)
		return ret;
	if (ret & I2SR_RX_NO_AK)
		return -EIO;
	return 0;
}

/*
 * Stop I2C transaction
 */
static void i2c_imx_stop(struct s32g_i2c_bus *i2c_bus)
{
	int ret;
	int reg_shift = i2c_bus->driver_data & I2C_QUIRK_FLAG ?
		VF610_I2C_REGSHIFT : IMX_I2C_REGSHIFT;
	unsigned long base = i2c_bus->base;
	unsigned int temp = mmio_read_8(base + (I2CR << reg_shift));

	temp &= ~(I2CR_MSTA | I2CR_MTX);
	mmio_write_8(base + (I2CR << reg_shift), temp);
	ret = wait_for_sr_state(i2c_bus, ST_BUS_IDLE);
	if (ret < 0)
		INFO("%s:trigger stop failed\n", __func__);
}

/*
 * Send start signal, chip address and
 * write register address
 */
static int i2c_init_transfer_(struct s32g_i2c_bus *i2c_bus, uint8_t chip,
		uint32_t addr, int alen)
{
	unsigned int temp;
	int ret;
	bool quirk = i2c_bus->driver_data & I2C_QUIRK_FLAG ? true : false;
	unsigned long base = i2c_bus->base;
	int reg_shift = quirk ? VF610_I2C_REGSHIFT : IMX_I2C_REGSHIFT;

	/* Enable I2C controller */
	if (quirk)
		ret = mmio_read_8(base + (I2CR << reg_shift)) & I2CR_IDIS;
	else
		ret = !(mmio_read_8(base + (I2CR << reg_shift)) & I2CR_IEN);

	if (ret) {
		mmio_write_8(base + (I2CR << reg_shift), I2CR_IEN);
		/* Wait for controller to be stable */
		udelay(50);
	}

	if (mmio_read_8(base + (IADR << reg_shift)) == (chip << 1))
		mmio_write_8(base + (IADR << reg_shift), (chip << 1) ^ 2);
	mmio_write_8(base + (I2SR << reg_shift), I2SR_IIF_CLEAR);
	ret = wait_for_sr_state(i2c_bus, ST_BUS_IDLE);
	if (ret < 0)
		return ret;

	/* Start I2C transaction */
	temp = mmio_read_8(base + (I2CR << reg_shift));
	temp |= I2CR_MSTA;
	mmio_write_8(base + (I2CR << reg_shift), temp);

	ret = wait_for_sr_state(i2c_bus, ST_BUS_BUSY);
	if (ret < 0)
		return ret;

	temp |= I2CR_MTX | I2CR_TX_NO_AK;
	mmio_write_8(base + (I2CR << reg_shift), temp);

	if (alen >= 0)	{
		/* write slave address */
		ret = tx_byte(i2c_bus, chip << 1);
		if (ret < 0)
			return ret;

		while (alen--) {
			ret = tx_byte(i2c_bus, (addr >> (alen * 8)) & 0xff);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int i2c_init_transfer(struct s32g_i2c_bus *i2c_bus, uint8_t chip,
		uint32_t addr, int alen)
{
	int retry;
	int ret;
	int reg_shift = i2c_bus->driver_data & I2C_QUIRK_FLAG ?
		VF610_I2C_REGSHIFT : IMX_I2C_REGSHIFT;

	if (!i2c_bus->base)
		return -EINVAL;

	for (retry = 0; retry < 3; retry++) {
		ret = i2c_init_transfer_(i2c_bus, chip, addr, alen);
		if (ret >= 0)
			return 0;
		i2c_imx_stop(i2c_bus);
		if (ret == -EIO)
			return ret;

		INFO("%s: failed for chip 0x%x retry=%d\n", __func__, chip,
				retry);
		if (ret != -EINTR)
			/* Disable controller */
			mmio_write_8(i2c_bus->base + (I2CR << reg_shift),
					I2CR_IDIS);
		udelay(100);
	}
	INFO("%s: give up i2c_regs=0x%lx\n", __func__, i2c_bus->base);
	return ret;
}


static int i2c_write_data(struct s32g_i2c_bus *i2c_bus, uint8_t chip,
		const uint8_t *buf, int len)
{
	int i, ret = 0;

	VERBOSE("%s : chip=0x%x, len=0x%x\n", __func__, chip, len);
	VERBOSE("%s: ", __func__);
	/* use rc for counter */
#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
	for (i = 0; i < len; ++i)
		printf(" 0x%02x", buf[i]);
	printf("\n");
#endif

	for (i = 0; i < len; i++) {
		ret = tx_byte(i2c_bus, buf[i]);
		if (ret < 0) {
			VERBOSE("%s: rc=%d\n", __func__, ret);
			break;
		}
	}

	return ret;
}

static int i2c_read_data(struct s32g_i2c_bus *i2c_bus, unsigned char chip,
		unsigned char *buf, int len)
{
	int ret;
	unsigned int temp;
	int i;
	int reg_shift = i2c_bus->driver_data & I2C_QUIRK_FLAG ?
		VF610_I2C_REGSHIFT : IMX_I2C_REGSHIFT;
	unsigned long base = i2c_bus->base;

	VERBOSE("%s: chip=0x%x, len=0x%x\n", __func__, chip, len);

	/* setup bus to read data */
	temp = mmio_read_8(base + (I2CR << reg_shift));
	temp &= ~(I2CR_MTX | I2CR_TX_NO_AK);
	if (len == 1)
		temp |= I2CR_TX_NO_AK;
	mmio_write_8(base + (I2CR << reg_shift), temp);
	mmio_write_8(base + (I2SR << reg_shift), I2SR_IIF_CLEAR);

	/* dummy read to clear ICF */
	mmio_read_8(base + (I2DR << reg_shift));

	/* read data */
	for (i = 0; i < len; i++) {
		ret = wait_for_sr_state(i2c_bus, ST_IIF);
		if (ret < 0) {
			VERBOSE("%s: ret=%d\n", __func__, ret);
			i2c_imx_stop(i2c_bus);
			return ret;
		}

		/*
		 * It must generate STOP before read I2DR to prevent
		 * controller from generating another clock cycle
		 */
		if (i == (len - 1)) {
			i2c_imx_stop(i2c_bus);
		} else if (i == (len - 2)) {
			temp = mmio_read_8(base + (I2CR << reg_shift));
			temp |= I2CR_TX_NO_AK;
			mmio_write_8(base + (I2CR << reg_shift), temp);
		}
		mmio_write_8(base + (I2SR << reg_shift), I2SR_IIF_CLEAR);
		buf[i] = mmio_read_8(base + (I2DR << reg_shift));
	}

#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
	/* reuse ret for counter*/
	for (ret = 0; ret < len; ++ret)
		printf(" 0x%02x", buf[ret]);
	printf("\n");
#endif
	i2c_imx_stop(i2c_bus);
	return 0;
}

/*
 * Read data from I2C device
 */
static int bus_i2c_read(struct s32g_i2c_bus *i2c_bus, uint8_t chip,
		uint32_t addr, int alen, uint8_t *buf, int len)
{
	int ret = 0;
	uint32_t temp;
	int reg_shift = i2c_bus->driver_data & I2C_QUIRK_FLAG ?
		VF610_I2C_REGSHIFT : IMX_I2C_REGSHIFT;
	unsigned long base = i2c_bus->base;

	ret = i2c_init_transfer(i2c_bus, chip, addr, alen);
	if (ret < 0)
		return ret;

	if (alen >= 0) {
		temp = mmio_read_8(base + (I2CR << reg_shift));
		temp |= I2CR_RSTA;
		mmio_write_8(base + (I2CR << reg_shift), temp);
	}

	ret = tx_byte(i2c_bus, (chip << 1) | 1);
	if (ret < 0) {
		i2c_imx_stop(i2c_bus);
		return ret;
	}

	ret = i2c_read_data(i2c_bus, chip, buf, len);

	i2c_imx_stop(i2c_bus);
	return ret;
}

/*
 * Write data to I2C device
 */
static int bus_i2c_write(struct s32g_i2c_bus *i2c_bus, uint8_t chip,
		uint32_t addr, int alen, const uint8_t *buf, int len)
{
	int ret = 0;

	ret = i2c_init_transfer(i2c_bus, chip, addr, alen);
	if (ret < 0)
		return ret;

	ret = i2c_write_data(i2c_bus, chip, buf, len);

	i2c_imx_stop(i2c_bus);

	return ret;
}

/*
 * I2C read
 * @bus:	I2C bus
 * @chip:	chip
 * @addr:	address
 * @alen:	data address length. This can be 1 or 2 bytes long.
 *		Some day it might be 3 bytes long.
 * @buffer:	buffer where data will be returned
 * @len:	number of objects
 */
int s32g_i2c_read(struct s32g_i2c_bus *bus, uint8_t chip,
		unsigned int addr, int alen, uint8_t *buffer,
		int len)
{
	if (!bus) {
		ERROR("bus_i2c_init: Invalid parameter bus\n");
		return -EINVAL;
	}
	if (alen > 3 || alen < 0) {
		ERROR("bus_i2c_init: Invalid parameter alen\n");
		return -EINVAL;
	}

	return bus_i2c_read(bus, chip, addr, alen, buffer, len);
}

/*
 * I2C write
 * @bus:	I2C bus
 * @chip:	chip
 * @addr:	address
 * @alen:	data address length. This can be 1 or 2 bytes long.
 *		Some day it might be 3 bytes long.
 * @buffer:	buffer where data will be returned
 * @len:	number of objects
 */
int s32g_i2c_write(struct s32g_i2c_bus *bus, uint8_t chip,
		unsigned int addr, int alen, uint8_t *buffer,
		int len)
{
	if (!bus) {
		ERROR("bus_i2c_init: Invalid parameter\n");
		return -EINVAL;
	}
	if (alen > 3 || alen < 0) {
		ERROR("bus_i2c_init: Invalid parameter alen\n");
		return -EINVAL;
	}

	return bus_i2c_write(bus, chip, addr, alen, buffer, len);
}

/*
 * Init I2C Bus
 */
int s32g_i2c_init(struct s32g_i2c_bus *bus)
{
	if (!bus) {
		ERROR("bus_i2c_init: Invalid parameter\n");
		return -EINVAL;
	}

	bus->driver_data = I2C_QUIRK_FLAG;
	bus->slaveaddr = S32G_DEFAULT_SLAVE;
	return bus_i2c_set_bus_speed(bus, bus->speed);
}

/*
 * @brief  Get I2C setup information from the device tree
 * @param  fdt: Pointer to the device tree
 * @param  node: I2C node offset
 * @param  bus: Ref to the initialization i2c_bus
 */
void s32g_i2c_get_setup_from_fdt(void *fdt, int node,
				 struct s32g_i2c_bus *bus)
{
	const fdt32_t *cuint;

	cuint = fdt_getprop(fdt, node, "reg", NULL);
	bus->base = cuint == NULL ? 0 : fdt32_to_cpu(*cuint);

	cuint = fdt_getprop(fdt, node, "clock-frequency", NULL);
	bus->speed = cuint == NULL ? S32G_DEFAULT_SPEED : fdt32_to_cpu(*cuint);
}

