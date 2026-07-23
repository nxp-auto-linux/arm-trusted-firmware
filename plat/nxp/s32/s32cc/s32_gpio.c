// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2026 NXP
 *
 * SIUL2 GPIO driver for S32CC -- plugs into the TF-A framework.
 */
#include <errno.h>

#include <common/debug.h>
#include <drivers/gpio.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include "platform_def.h"
#include "s32cc_gpio.h"
#include "s32cc_pinctrl.h"

#define SIUL2_GPDO_OFFSET	U(0x1300)
#define SIUL2_GPDI_OFFSET	U(0x1500)

/*
 * GPDO and GPDI are arrays of 8-bit registers, one register per pad,
 * holding the output (PDO) and input (PDI) value in bit 0.
 */
#define SIUL2_GPDO(N)		((N) ^ U(3))

static inline uint32_t siul2_get_pad_offset(uint32_t pin)
{
	return SIUL2_GPDO(pin);
}

struct s32_gpio_range {
	uint16_t	start;	/* first global pin in this SIUL2 instance  */
	uint16_t	end;	/* last  global pin in this SIUL2 instance  */
	uintptr_t	base;	/* SIUL2 instance base address              */
};

/*
 * Resolve a global pin into the SIUL2 instance that owns it and the
 * local pin index within that instance. Returns 0 on success.
 */
static int s32_gpio_resolve(uint16_t gpio, uintptr_t *base, uint16_t *local)
{
	static const struct s32_gpio_range ranges[] = {
		{
			.start = SIUL2_0_MSCR_START,
			.end   = SIUL2_0_MSCR_END,
			.base  = SIUL2_0_BASE_ADDR,
		},
		{
			.start = SIUL2_1_MSCR_START,
			.end   = SIUL2_1_MSCR_END,
			.base  = SIUL2_1_BASE_ADDR,
		},
	};
	unsigned int i;

	for (i = 0U; i < ARRAY_SIZE(ranges); i++) {
		const struct s32_gpio_range *r = &ranges[i];

		if (gpio >= r->start && gpio <= r->end) {
			*base  = r->base;
			*local = gpio - r->start;
			return 0;
		}
	}

	return -EINVAL;
}

/*
 * Common helper: resolve a gpio and compute the GPDO/GPDI register address
 * for that pin. @gpd_offset selects the GPDO or GPDI bank.
 * Returns 0 on success; on error logs and returns negative errno.
 */
static int s32_gpio_gpd_resolve(int gpio, uintptr_t gpd_offset,
				uintptr_t *reg_addr)
{
	uintptr_t base;
	uint16_t local;
	int ret;

	if (gpio < 0 || gpio > UINT16_MAX) {
		ERROR("%s: invalid gpio number %d\n", __func__, gpio);
		return -EINVAL;
	}

	ret = s32_gpio_resolve((uint16_t)gpio, &base, &local);
	if (ret) {
		ERROR("%s: invalid gpio %d\n", __func__, gpio);
		return ret;
	}

	*reg_addr = base + gpd_offset + siul2_get_pad_offset(local);
	return 0;
}

/*
 * Pin direction lives in the MSCR register, which is pinctrl territory.
 * Delegate both callbacks to the pinctrl driver so all MSCR access
 * stays inside the pinctrl driver.
 */
static int s32_gpio_get_direction(int gpio)
{
	return s32_pinctrl_get_direction(gpio);
}

static void s32_gpio_set_direction(int gpio, int direction)
{
	s32_pinctrl_set_direction(gpio, direction);
}

static int s32_gpio_get_value(int gpio)
{
	uintptr_t reg_addr;
	int ret;

	ret = s32_gpio_gpd_resolve(gpio, SIUL2_GPDI_OFFSET, &reg_addr);
	if (ret)
		return ret;

	return (mmio_read_8(reg_addr) & 1U) ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;
}

static void s32_gpio_set_value(int gpio, int value)
{
	uintptr_t reg_addr;
	int ret;

	ret = s32_gpio_gpd_resolve(gpio, SIUL2_GPDO_OFFSET, &reg_addr);
	if (ret)
		return;

	mmio_write_8(reg_addr, (value == GPIO_LEVEL_HIGH) ? 1U : 0U);
}

static const gpio_ops_t s32_gpio_ops = {
	.get_direction = s32_gpio_get_direction,
	.set_direction = s32_gpio_set_direction,
	.get_value     = s32_gpio_get_value,
	.set_value     = s32_gpio_set_value,
};

int s32_gpio_init(void)
{
	int ret;

	/* The MSCR/GPDO/GPDI registers live inside the SIUL2 banks.
	 * Make sure the dynamic mappings exist before any ops_t entry
	 * touches them.
	 */
	ret = s32_mmap_siul2_regions();
	if (ret) {
		ERROR("s32_gpio: failed to mmap SIUL2 regions: %d\n", ret);
		return ret;
	}

	gpio_init(&s32_gpio_ops);
	return 0;
}
