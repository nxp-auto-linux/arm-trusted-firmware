/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32G274A_I2C_H
#define S32G274A_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <common/debug.h>
#include <lib/utils_def.h>

#define S32G_I2C4_SPEED 100000
#define S32G_I2C4_SLAVE 0

/*
 * Information about i2c controller
 * struct s32g_i2c_bus - information about the i2c bus
 * @index: i2c bus index
 * @base: Address of I2C bus controller
 * @driver_data: Flags for different platforms, such as I2C_QUIRK_FLAG.
 * @speed: Speed of I2C bus
 * @idle_bus_fn: function to force bus idle
 * @idle_bus_data: parameter for idle_bus_fun
 */
struct s32g_i2c_bus {
	unsigned long	base;
	unsigned long	driver_data;
	int             speed;
	int             slaveaddr;
};

int s32g_i2c_init(struct s32g_i2c_bus *bus);
int s32g_i2c_read(struct s32g_i2c_bus *bus, uint8_t chip,
		unsigned int addr, int alen, uint8_t *buffer,
		int len);
int s32g_i2c_write(struct s32g_i2c_bus *bus, uint8_t chip,
		unsigned int addr, int alen, uint8_t *buffer,
		int len);

#endif
