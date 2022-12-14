// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020 NXP
 */

#ifndef S32G274A_I2C_H
#define S32G274A_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <common/debug.h>
#include <lib/utils_def.h>

#define S32G_DEFAULT_SPEED 100000
#define S32G_DEFAULT_SLAVE 0

/*
 * I2C bus description
 * @base: I2C bus controller address
 * @speed: I2C bus speed
 */
struct s32g_i2c_bus {
	unsigned long	base;
	int             speed;
};

void s32g_i2c_get_setup_from_fdt(void *fdt, int node, struct s32g_i2c_bus *bus);
int s32g_i2c_init(struct s32g_i2c_bus *bus);
uint8_t s32g_i2c_read(struct s32g_i2c_bus *bus, uint8_t chip,
		unsigned int addr, int addr_len, uint8_t *buffer,
		int len);
uint8_t s32g_i2c_write(struct s32g_i2c_bus *bus, uint8_t chip,
		unsigned int addr, int addr_len, uint8_t *buffer,
		int len);

#endif
