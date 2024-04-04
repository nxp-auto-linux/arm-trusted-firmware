// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/*
 * Copyright 2020-2021, 2024 NXP
 */
#include <common/debug.h>
#include <endian.h>
#include <errno.h>
#include <lib/crc8.h>
#include <lib/utils_def.h>
#include <libfdt.h>
#include <stdint.h>
#include <stdlib.h>

#include "pmic/vr5510.h"
#include "s32cc_dt.h"

#define VR5510_ADDR_SIZE	2
#define VR5510_REG_SIZE		2
#define VR5510_CRC_SIZE		1

#define VR5510_CRC_POLY		0x1Du
#define VR5510_CRC_SEED		0xFFu

#define DEV_ADDR_MASK		0xFE00U
#define DEV_ADDR_SHIFT		9
#define DEV_RW_MASK		0x100U
#define DEV_RW_SHIFT		8

/* Includes zeros from 8-6 */
#define REG_ADDR_MASK		0xFF
#define REG_ADDR_SHIFT		0

#define VR5510_M_LVB1_STBY_DVS_ID	17
#define VR5510_M_MEMORY0_ID		41

#define VF5510_MU_N_REGS		43
#define VF5510_FSU_N_REGS		24

#define VR5510_ADDRESS_LENGTH		1

#define MAX_VR5510_INSTANCES	2
#define MAX_NAME_LEN		30

struct vr5510_inst {
	struct dt_node_info dt_info;
	struct s32_i2c_bus *bus;
	char name[MAX_NAME_LEN];
	int fdt_offset;
	uint8_t chip;
};

static struct vr5510_inst instances[MAX_VR5510_INSTANCES];
static size_t fill_level;

struct read_msg {
	uint16_t address;
	uint16_t data;
	uint8_t crc;
};

static bool is_mu(struct vr5510_inst *dev)
{
	if (dev->chip & 1)
		return false;

	return true;
}

static bool valid_register(struct vr5510_inst *dev, uint8_t reg)
{
	/* There are no gaps in FSU */
	if (!is_mu(dev))
		return true;

	if (reg > VR5510_M_LVB1_STBY_DVS_ID && reg < VR5510_M_MEMORY0_ID)
		return false;

	return true;
}

static void set_dev_addr(struct read_msg *m, uint8_t addr)
{
	m->address &= ~DEV_ADDR_MASK;
	m->address |= (addr << DEV_ADDR_SHIFT);
}

static void set_rw(struct read_msg *m, bool read)
{
	m->address &= ~DEV_RW_MASK;
	if (read)
		m->address |= DEV_RW_MASK;
}

static void set_reg_addr(struct read_msg *m, uint8_t addr)
{
	m->address &= ~REG_ADDR_MASK;
	m->address |= (addr << REG_ADDR_SHIFT);
}

static int vr5510_i2c_read(struct vr5510_inst *dev, uint8_t reg,
			   uint8_t *data, size_t len)
{
	return s32_i2c_read(dev->bus, dev->chip, reg,
			     VR5510_ADDRESS_LENGTH, data, len);
}

static int vr5510_i2c_write(struct vr5510_inst *dev, uint8_t reg,
			    uint8_t *data, size_t len)
{
	return s32_i2c_write(dev->bus, dev->chip, reg,
			      VR5510_ADDRESS_LENGTH, data, len);
}

int vr5510_read(struct vr5510_inst *dev, uint8_t reg, uint8_t *buff, int len)
{
	unsigned int crc;
	struct read_msg msg = {.address = 0, .data = 0, .crc = 0};

	if (!valid_register(dev, reg)) {
		ERROR("Invalid vr5510 register %d\n", reg);
		return -EIO;
	}

	set_dev_addr(&msg, dev->chip);
	set_rw(&msg, true);
	set_reg_addr(&msg, reg);
	msg.address = bswap16(msg.address);

	if (vr5510_i2c_read(dev, reg, (uint8_t *)&msg.data,
			VR5510_REG_SIZE + VR5510_CRC_SIZE)) {
		ERROR("read error from device: %p register: %#x!\n", dev, reg);
		return -EIO;
	}

	crc = crc8poly(VR5510_CRC_SEED, VR5510_CRC_POLY,
		       (const unsigned char *)&msg,
		       VR5510_ADDR_SIZE + VR5510_REG_SIZE);

	if (crc != msg.crc) {
		ERROR("read error from device: %p register: %#x!\n", dev, reg);
		return -EIO;
	}

	*(uint16_t *)buff = bswap16(msg.data);

	return 0;
}

int vr5510_write(struct vr5510_inst *dev, uint8_t reg,
		 const uint8_t *buff, int len)
{
	struct read_msg msg = {.address = 0, .data = 0, .crc = 0};

	if (!valid_register(dev, reg)) {
		ERROR("Invalid vr5510 register %d\n", reg);
		return -EIO;
	}

	set_dev_addr(&msg, dev->chip);
	set_rw(&msg, false);
	set_reg_addr(&msg, reg);
	msg.data = bswap16(*(uint16_t *)buff);
	msg.address = bswap16(msg.address);

	msg.crc = crc8poly(VR5510_CRC_SEED, VR5510_CRC_POLY,
			   (const unsigned char *)&msg,
			   VR5510_ADDR_SIZE + VR5510_REG_SIZE);

	if (vr5510_i2c_write(dev, reg, (uint8_t *)&msg.data,
			 VR5510_REG_SIZE + VR5510_CRC_SIZE)) {
		ERROR("write error to device: %p register: %#x!\n", dev, reg);
		return -EIO;
	}

	return 0;
}

int vr5510_get_inst(const char *name, vr5510_t *inst)
{
	size_t i;
	size_t len = strlen(name);

	for (i = 0; i < fill_level; i++) {
		*inst = &instances[i];
		if (strncmp(name, (*inst)->name, len))
			continue;

		if ((*inst)->name[len] != '@')
			continue;

		return 0;
	}

	*inst = NULL;
	return -1;
}

int vr5510_register_instance(void *fdt, int fdt_offset,
			     struct s32_i2c_bus *bus)
{
	size_t i;
	struct vr5510_inst *inst;
	const fdt32_t *reg_ptr;

	for (i = 0; i < fill_level; i++) {
		if (instances[i].fdt_offset == fdt_offset)
			return 0;
	}

	if (fill_level >= ARRAY_SIZE(instances)) {
		ERROR("Discovered too many instances of VR5510\n");
		return -ENOMEM;
	}

	inst = &instances[fill_level];

	/* Register active nodes only */
	dt_fill_device_info(&inst->dt_info, fdt_offset);
	if (inst->dt_info.status != DT_ENABLED)
		return -1;

	reg_ptr = fdt_getprop(fdt, fdt_offset, "reg", NULL);
	if (!reg_ptr) {
		ERROR("\"reg\" property is mandatory\n");
		return -EIO;
	}

	inst->chip = fdt32_to_cpu(*reg_ptr);
	strlcpy(inst->name, fdt_get_name(fdt, fdt_offset, NULL),
		sizeof(inst->name));
	inst->bus = bus;
	inst->fdt_offset = fdt_offset;

	fill_level++;
	return 0;
}
