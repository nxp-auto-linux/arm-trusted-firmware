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

#define SIUL2_PGPDO_OFFSET	U(0x1700)
#define SIUL2_PGPDI_OFFSET	U(0x1740)

#define SIUL2_GPIO_16_PAD_SIZE	U(16)

/* PGPDO/PGPDI expose 16-bit port views with swapped half-word ordering. */
#define SIUL2_PGPDO(N)		(((N) ^ U(1)) * U(2))

/*
 * siul2_pin2mask: bit mask for @pin within a 16-bit PGPDO/PGPDI half-word.
 * siul2_pin2pad: pad index of @pin (one pad covers 16 pins).
 * siul2_get_pad_offset: byte offset of a pad within the PGPDO/PGPDI bank
 */
static inline uint16_t siul2_pin2mask(uint16_t pin)
{
	return (uint16_t)BIT_32(15U - (pin % SIUL2_GPIO_16_PAD_SIZE));
}

static inline uint32_t siul2_pin2pad(uint16_t pin)
{
	return pin / SIUL2_GPIO_16_PAD_SIZE;
}

static inline uint32_t siul2_get_pad_offset(uint32_t pad)
{
	return SIUL2_PGPDO(pad);
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
 * Common helper: resolve a gpio and compute the PGPDO/PGPDI register address
 * and the bit mask for that pin. @pgp_offset selects PGPDO or PGPDI bank.
 * Returns 0 on success; on error logs and returns negative errno.
 */
static int s32_gpio_pgp_resolve(int gpio, uintptr_t pgp_offset,
				uintptr_t *reg_addr, uint16_t *mask)
{
	uintptr_t base;
	uint16_t local;
	uint32_t pad;
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

	pad = siul2_pin2pad(local);
	*mask = siul2_pin2mask(local);
	*reg_addr = base + pgp_offset + siul2_get_pad_offset(pad);
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
	uint16_t mask;
	int ret;

	ret = s32_gpio_pgp_resolve(gpio, SIUL2_PGPDI_OFFSET, &reg_addr, &mask);
	if (ret)
		return ret;

	return (mmio_read_16(reg_addr) & mask) ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;
}

static void s32_gpio_set_value(int gpio, int value)
{
	uintptr_t reg_addr;
	uint16_t mask, val;
	int ret;

	ret = s32_gpio_pgp_resolve(gpio, SIUL2_PGPDO_OFFSET, &reg_addr, &mask);
	if (ret)
		return;

	val = mmio_read_16(reg_addr);
	if (value == GPIO_LEVEL_HIGH)
		val |= mask;
	else
		val &= (uint16_t)~mask;
	mmio_write_16(reg_addr, val);
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

	/* The MSCR/PGPDO/PGPDI registers live inside the SIUL2 banks.
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
