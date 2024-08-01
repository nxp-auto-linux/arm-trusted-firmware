// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022, 2024 NXP
 */

#include <common/fdt_wrappers.h>
#include <stdio.h>
#include <lib/mmio.h>
#include <libfdt.h>
#include <drivers/delay_timer.h>
#include <drivers/nxp/s32/i2c/s32_i2c.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include "s32cc_clocks.h"

/* Register index */
#define IBFD	1
#define IBCR	2
#define IBSR	3
#define IBDR	4

#define IBSR_RXAK	BIT(0)
#define IBSR_IBIF	BIT(1)
#define IBSR_IBAL	BIT(4)
#define IBSR_IBB	BIT(5)

#define IBCR_RSTA	BIT(2)
#define IBCR_NOACK	BIT(3)
#define IBCR_TX		BIT(4)
#define IBCR_MSSL	BIT(5)

#define IBSR_IBIF_CLEAR	BIT(1)
#define IBCR_MDIS_EN	(0 << 7)
#define IBCR_MDIS_DIS	BIT(7)

#define I2C_READ	1
#define I2C_WRITE	0

#define I2C_MAX_RETRY_CNT               10

#define I2C_IDLE			0
#define I2C_TRANSMISSION_COMPLETE	2

static uint16_t s32_clk_div[] = {
	20,   22,   24,   26,   28,   30,   34,   40,   28,   32,
	36,   40,   44,   48,   58,   64,   48,   56,   64,   72,
	80,   88,   104,  128,  80,   96,   112,  128,  144,  160,
	192,  240,  160,  192,  224,  256,  288,  320,  384,  480,
	320,  384,  448,  512,  576,  640,  768,  960,  640,  768,
	896,  1024, 1152, 1280, 1536, 1920, 1280, 1536, 1792, 2048,
	2304, 2560, 3072, 3840
};

static inline void s32_i2c_disable(struct s32_i2c_bus *bus)
{
	mmio_write_8(bus->base + IBCR, IBCR_MDIS_DIS);
	mmio_write_8(bus->base + IBSR, 0);
}

static inline void s32_i2c_enable(struct s32_i2c_bus *bus)
{
	mmio_write_8(bus->base + IBCR, IBCR_MDIS_EN);
	/* Clear interrupt flag */
	mmio_write_8(bus->base + IBSR, IBSR_IBIF_CLEAR);
}

/*
 * Configure bus speed
 */
static int s32_i2c_set_bus_speed(struct s32_i2c_bus *bus, int speed)
{
	int i;

	if (!bus || !bus->base)
		return -EINVAL;

	for (i = 0; ARRAY_SIZE(s32_clk_div) - 1; i++)
		if ((I2C_CLK_FREQ / s32_clk_div[i]) <= speed)
			break;

	/* Write divider value */
	mmio_write_8(bus->base + IBFD, i);

	/* Module reset */
	s32_i2c_disable(bus);
	return 0;
}

/*
 * Wait until the bus enters a specified state or timeout occurs.
 */
static int s32_i2c_wait(struct s32_i2c_bus *bus, unsigned int state)
{
	uint8_t ibsr;
	uint32_t wait_cnt = 1000;

	while (wait_cnt--) {
		ibsr = mmio_read_8(bus->base + IBSR);

		switch(state) {
		case I2C_IDLE:
			if (!(ibsr & IBSR_IBB))
				return 0;
			break;
		case I2C_TRANSMISSION_COMPLETE:
			if (ibsr & IBSR_IBIF) {
				/* Clear interrupt flag */
				mmio_write_8(bus->base + IBSR,
					     IBSR_IBIF_CLEAR);
				return 0;
			}
		}

		udelay(1);
	}

	INFO("%s: timeout state=%x\n", __func__, state);
	return -ETIMEDOUT;
}

static int s32_i2c_write_byte(struct s32_i2c_bus *bus, uint8_t byte)
{
	uint8_t ibsr;
	int ret;

	if (!bus || !bus->base)
		return -EINVAL;

	/* Write data */
	mmio_write_8(bus->base + IBDR, byte);

	/* Wait for transfer complete */
	ret = s32_i2c_wait(bus, I2C_TRANSMISSION_COMPLETE);
	if (ret < 0)
		return ret;

	/* Examine IBSR[RXAK] for an acknowledgment from the slave. */
	ibsr = mmio_read_8(bus->base + IBSR);
	return ibsr & IBSR_RXAK ? -EIO : 0;
}

static int s32_i2c_chip_setup(struct s32_i2c_bus *bus,
			      uint8_t chip, int mode)
{
	/* The master transmits the seven-bit slave address.
	 * The master transmits the R/W bit.
	 */
	return s32_i2c_write_byte(bus, (chip << 1) | mode);
}

static int s32_i2c_address_setup(struct s32_i2c_bus *bus,
				 uint32_t addr, int addr_len)
{
	uint8_t reg;
	int ret;

	while (addr_len--) {
		/* Write data to I2C Bus Data I/O Register (IBDR) */
		reg = (addr >> (addr_len * 8)) & 0xff;
		ret = s32_i2c_write_byte(bus, reg);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/*
 * Stop sequence
 */
static uint8_t s32_i2c_stop(struct s32_i2c_bus *bus)
{
	uint8_t ibcr;
	int ret;

	if (!bus || !bus->base)
		return -EINVAL;

	/* Clear IBCR */
	ibcr = mmio_read_8(bus->base + IBCR) & ~IBCR_MSSL;
	mmio_write_8(bus->base + IBCR, ibcr);

	/* Wait for idle state */
	ret = s32_i2c_wait(bus, I2C_IDLE);
	if (ret == -ETIMEDOUT)
		s32_i2c_disable(bus);
	return ret;
}

/*
 * Prepare the transfer by sending: start signal, chip and write
 * register address
 */
static int s32_i2c_try_start(struct s32_i2c_bus *bus,
			     uint8_t chip, uint32_t addr, int addr_len)
{
	uint8_t reg;
	int ret;

	if (!bus || !bus->base)
		return -EINVAL;

	/* Clear the MDIS field to enable the I2C interface system */
	s32_i2c_disable(bus);
	s32_i2c_enable(bus);

	/* Wait in loop for IBB flag to clear. */
	ret = s32_i2c_wait(bus, I2C_IDLE);
	if (ret < 0)
		return ret;

	/* Set as master */
	reg = mmio_read_8(bus->base + IBCR) | IBCR_MSSL;
	mmio_write_8(bus->base + IBCR, reg);

	/* Set transmission and noack */
	reg |= IBCR_TX | IBCR_NOACK;
	mmio_write_8(bus->base + IBCR, reg);

	/* Send chip and address */
	ret = s32_i2c_chip_setup(bus, chip, I2C_WRITE);
	if (ret < 0)
		return ret;

	return s32_i2c_address_setup(bus, addr, addr_len);
}

/*
 * Start sequence
 */
static int s32_i2c_start(struct s32_i2c_bus *bus, uint8_t chip,
		uint32_t addr, int addr_len)
{
	int counter = 0;
	int ret = 0;

	if (!bus || !bus->base)
		return -EINVAL;

	do {
		if (counter++ > 0)
			udelay(100);

		ret = s32_i2c_try_start(bus, chip, addr, addr_len);
		if (ret >= 0)
			return 0;

		s32_i2c_stop(bus);
	} while ((ret == -EAGAIN) && (counter < I2C_MAX_RETRY_CNT));

	INFO("%s: failed\n", __func__);
	return ret;
}

static int s32_i2c_read_buffer(struct s32_i2c_bus *bus, unsigned char chip,
			       unsigned char *buf, int len)
{
	int i, ret;
	uint8_t reg;

	if (!bus || !bus->base || !buf)
		return -EINVAL;

	if (!len)
		return 0;

	/* Perform a dummy read of IBDR to initiate the receive operation */
	mmio_read_8(bus->base + IBDR);

	/* Read data */
	for (i = 0; i < len; i++) {
		/* Wait for transfer complete. */
		ret = s32_i2c_wait(bus, I2C_TRANSMISSION_COMPLETE);
		if (ret < 0)
			return ret;

		if (i == (len - 2)) {
			/* Disable ACK */
			reg = mmio_read_8(bus->base + IBCR) | IBCR_NOACK;
			mmio_write_8(bus->base + IBCR, reg);
		} else if (i == (len - 1)) {
			/* Generate STOP condition */
			reg = mmio_read_8(bus->base + IBCR) & ~IBCR_MSSL;
			mmio_write_8(bus->base + IBCR, reg);
		}

		/* Read data */
		buf[i] = mmio_read_8(bus->base + IBDR);
	}

	return 0;
}

/*
 * I2C read
 * @bus:	I2C bus
 * @chip:	chip
 * @addr:	address
 * @addr_len:	data address length. This can be 1 or 2 bytes long.
 *		Some day it might be 3 bytes long.
 * @buffer:	buffer where data will be returned
 * @len:	number of objects
 */
int s32_i2c_read(struct s32_i2c_bus *bus, uint8_t chip,
		 unsigned int addr, int addr_len, uint8_t *buffer,
		 int len)
{
	uint8_t reg;
	int ret = 0;

	if (!bus || !bus->base || !buffer) {
		ERROR("%s: Invalid parameter\n", __func__);
		return -EINVAL;
	}
	if (addr_len > 3 || addr_len <= 0) {
		ERROR("%s: Invalid parameter addr_len\n", __func__);
		return -EINVAL;
	}

	ret = s32_i2c_start(bus, chip, addr, addr_len);
	if (ret < 0)
		return ret;

	/* Generate repeat start condition */
	reg = mmio_read_8(bus->base + IBCR) | IBCR_RSTA;
	mmio_write_8(bus->base + IBCR, reg);

	/* Setup in read mode. */
	ret = s32_i2c_chip_setup(bus, chip, I2C_READ);
	if (ret < 0) {
		s32_i2c_stop(bus);
		return ret;
	}

	/* Select Receive mode */
	reg = mmio_read_8(bus->base + IBCR) & ~IBCR_TX;

	/* No ack necessary if only one byte is sent */
	if (len == 1)
		reg |= IBCR_NOACK;
	else
		reg &= ~IBCR_NOACK;

	mmio_write_8(bus->base + IBCR, reg);

	ret = s32_i2c_read_buffer(bus, chip, buffer, len);

	s32_i2c_stop(bus);
	return ret;
}

/*
 * I2C write
 * @bus:	I2C bus
 * @chip:	chip
 * @addr:	address
 * @addr_len:	data address length. This can be 1 or 2 bytes long.
 *		Some day it might be 3 bytes long.
 * @buffer:	buffer where data will be returned
 * @len:	number of objects
 */
int s32_i2c_write(struct s32_i2c_bus *bus, uint8_t chip,
		  unsigned int addr, int addr_len, uint8_t *buffer,
		  int len)
{
	int ret = 0;
	int i;

	if (!bus || !bus->base || !buffer) {
		ERROR("%s: Invalid parameter\n", __func__);
		return -EINVAL;
	}
	if (addr_len > 3 || addr_len <= 0) {
		ERROR("%s: Invalid parameter addr_len\n", __func__);
		return -EINVAL;
	}

	ret = s32_i2c_start(bus, chip, addr, addr_len);
	if (ret < 0)
		return ret;

	/* Start the transfer */
	for (i = 0; i < len; i++) {
		ret = s32_i2c_write_byte(bus, buffer[i]);
		if (ret < 0)
			break;
	}

	s32_i2c_stop(bus);
	return ret;
}

/*
 * Init I2C Bus
 */
int s32_i2c_init(struct s32_i2c_bus *bus)
{
	size_t reg_size;
	int ret;

	if (!bus) {
		ERROR("%s: Invalid parameter\n", __func__);
		return -EINVAL;
	}

	if (!bus->mmap_added) {
		reg_size = round_up(bus->reg_size, PAGE_SIZE);
		ret = mmap_add_dynamic_region(bus->base, bus->base, reg_size, MT_DEVICE | MT_RW);

		if (ret) {
			ERROR("Failed to map I2C bus region. base_pa: 0x%lx, error: %d\n",
			      (unsigned long)bus->base, ret);
			return ret;
		}
		bus->mmap_added = true;
	}

	return s32_i2c_set_bus_speed(bus, bus->speed);
}

/*
 * @brief  Get I2C setup information from the device tree
 * @param  fdt: Pointer to the device tree
 * @param  node: I2C node offset
 * @param  bus: Ref to the initialization i2c_bus
 */
void s32_i2c_get_setup_from_fdt(void *fdt, int node,
				struct s32_i2c_bus *bus)
{
	const fdt32_t *cuint;
	int ret;

	ret = fdt_get_reg_props_by_index(fdt, node, 0, &bus->base, &bus->reg_size);
	if (ret) {
		ERROR("Invalid I2C base address\n");
		return;
	}

	cuint = fdt_getprop(fdt, node, "clock-frequency", NULL);
	bus->speed = cuint == NULL ? S32_DEFAULT_SPEED : fdt32_to_cpu(*cuint);

	bus->mmap_added = false;
}

