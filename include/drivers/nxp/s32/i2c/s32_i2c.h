// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2022, 2024, 2026 NXP
 */

#ifndef S32_I2C_H
#define S32_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <common/debug.h>
#include <lib/utils_def.h>

#define S32_DEFAULT_SPEED 100000
#define S32_DEFAULT_SLAVE 0

struct s32_peripheral_config;

/*
 * Per-bus information used by the ERR050686 bus-recovery routine.
 *
 * @scl_pin / @sda_pin : SIUL2 pin numbers for SCL and SDA.
 * @gpio_cfg           : peripheral pinmux that puts SCL/SDA into the GPIO
 *                       function with open-drain output enabled (mirrors the
 *                       i2c<n>_gpio_pins state from the device tree). The
 *                       recovery code applies this before bit-banging.
 * @restore_cfg        : peripheral pinmux that puts SCL/SDA back into the
 *                       I2C function. Reapplied at the end of recovery.
 *
 * Platforms register this with s32_i2c_set_recovery_info() after the bus
 * has been discovered (e.g. from FDT).
 */
struct s32_i2c_recovery {
	uint16_t scl_pin;
	uint16_t sda_pin;
	const struct s32_peripheral_config *gpio_cfg;
	const struct s32_peripheral_config *restore_cfg;
};

/*
 * I2C bus description
 * @base: I2C bus controller address
 * @reg_size: I2C node size parsed from fdt
 * @speed: I2C bus speed
 * @recovery: optional bus-recovery descriptor for ERR050686
 */
struct s32_i2c_bus {
	uintptr_t			base;
	size_t				reg_size;
	int				speed;
	bool				mmap_added;
	struct s32_i2c_recovery		recovery;
};

void s32_i2c_get_setup_from_fdt(void *fdt, int node, struct s32_i2c_bus *bus);
int s32_i2c_init(struct s32_i2c_bus *bus);
int s32_i2c_read(struct s32_i2c_bus *bus, uint8_t chip,
		 unsigned int addr, int addr_len, uint8_t *buffer,
		 int len);
int s32_i2c_write(struct s32_i2c_bus *bus, uint8_t chip,
		  unsigned int addr, int addr_len, uint8_t *buffer,
		  int len);

/*
 * Attach the GPIO recovery descriptor used by ERR050686 workaround.
 */
void s32_i2c_set_recovery_info(struct s32_i2c_bus *bus,
			       const struct s32_i2c_recovery *recovery);

#endif
