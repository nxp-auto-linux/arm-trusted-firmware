// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */

#include <stdint.h>
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <drivers/delay_timer.h>
#include <drivers/nxp/s32/adc/s32_adc.h>
#include <lib/libc/errno.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <stddef.h>

#define SARADC_MCR		0x0
#define SARADC_MSR		0x4
#define SARADC_ISR		0x10
#define SARADC_CTR0		0x94
#define SARADC_NCMR0		0xA4
#define SARADC_PCDR(x)		(0x100 + (x) * 4)

#define SARADC_MCR_PWDN		BIT_32(0)
#define SARADC_MCR_ADCLKSE	BIT_32(8)
#define SARADC_MCR_TSAMP_MASK	GENMASK(10, 9)
#define SARADC_MCR_8_SAMP	BIT_32(9)
#define SARADC_MCR_NRSMPL_MASK	GENMASK(12, 11)
#define SARADC_MCR_AVGEN	BIT_32(13)
#define SARADC_MCR_CALSTART	BIT_32(14)
#define SARADC_MCR_NSTART	BIT_32(24)
#define SARADC_MCR_SCAN_MODE	BIT_32(29)
#define SARADC_MCR_WLSIDE	BIT_32(30)
#define SARADC_MCR_OWREN	BIT_32(31)

#define SARADC_MSR_CALBUSY	BIT_32(29)
#define SARADC_MSR_CALFAIL	BIT_32(30)

#define SARADC_ISR_ECH		BIT_32(0)

#define SARADC_CTR0_INPSAMP(x)	(x)

#define SARADC_PCDR_VALID	BIT_32(19)
#define SARADC_PCDR_DATA_MASK	GENMASK(11, 0)
#define SARADC_PCDR_CDATA(x)	((x) & SARADC_PCDR_DATA_MASK)

#define SARADC_NSEC_PER_SEC	1000000000

static void s32_saradc_powerdown(unsigned long base)
{
	uint32_t mcr;

	mcr = mmio_read_32(base + SARADC_MCR) | SARADC_MCR_PWDN;
	mmio_write_32(base + SARADC_MCR, mcr);
}

static void s32_saradc_powerup(unsigned long base)
{
	uint32_t mcr;

	mcr = mmio_read_32(base + SARADC_MCR) & ~SARADC_MCR_PWDN;
	mmio_write_32(base + SARADC_MCR, mcr);
}

static int s32_saradc_calibration(struct s32_adc *adc)
{
	uint32_t us_left = 300;
	uint32_t mcr, msr;

	s32_saradc_powerdown(adc->base);

	/* Configure clock = bus_clock / 2 */
	mcr = mmio_read_32(adc->base + SARADC_MCR) & ~SARADC_MCR_ADCLKSE;
	mmio_write_32(adc->base + SARADC_MCR, mcr);

	s32_saradc_powerup(adc->base);

	mcr = mmio_read_32(adc->base + SARADC_MCR);
	mcr |= SARADC_MCR_AVGEN;
	mcr &= ~SARADC_MCR_NRSMPL_MASK;
	mcr &= ~SARADC_MCR_TSAMP_MASK;
	mcr |= SARADC_MCR_8_SAMP;
	mcr |= SARADC_MCR_CALSTART;
	mmio_write_32(adc->base + SARADC_MCR, mcr);

	do {
		msr = mmio_read_32(adc->base + SARADC_MSR);
		udelay(1);
		us_left--;
	} while (us_left && (msr & SARADC_MSR_CALBUSY));

	if (!us_left)
		return -ETIMEDOUT;

	if (msr & SARADC_MSR_CALFAIL)
		return -EIO;

	return 0;
}

static int s32_adc_start_channel(struct s32_adc *adc, uint8_t channel)
{
	uint32_t mcr;

	if (!adc)
		return -EINVAL;

	if (channel >= adc->num_channels)
		return -EINVAL;

	s32_saradc_powerup(adc->base);

	mmio_write_32(adc->base + SARADC_NCMR0, BIT_32(channel));

	/* Ensure there are at least three cycles between the
	 * configuration of NCMR and the setting of NSTART
	 */
	udelay(1);

	mcr = mmio_read_32(adc->base + SARADC_MCR);
	mcr |= SARADC_MCR_OWREN;
	mcr &= ~SARADC_MCR_WLSIDE;
	mcr &= ~SARADC_MCR_SCAN_MODE;

	mcr |= SARADC_MCR_NSTART;
	mmio_write_32(adc->base + SARADC_MCR, mcr);

	return 0;
}

int s32_adc_read_channel(struct s32_adc *adc, uint8_t channel,
			 uint32_t *data)
{
	uint32_t pcdr, us_left = 10;
	int ret;

	if (!adc)
		return -EINVAL;

	if (channel >= adc->num_channels)
		return -EINVAL;

	ret = s32_adc_start_channel(adc, channel);
	if (ret)
		return ret;

	while (!(mmio_read_32(adc->base + SARADC_ISR) & SARADC_ISR_ECH) &&
	       us_left) {
		us_left--;
		udelay(1);
	};

	if (!us_left)
		return -ETIMEDOUT;

	mmio_write_32(adc->base + SARADC_ISR, SARADC_ISR_ECH);

	pcdr = mmio_read_32(adc->base + SARADC_PCDR(channel));
	if (!(pcdr & SARADC_PCDR_VALID))
		return -EBUSY;

	*data = SARADC_PCDR_CDATA(pcdr);

	s32_saradc_powerdown(adc->base);

	return 0;
}

int s32_adc_init(struct s32_adc *adc)
{
	uint32_t mcr;
	int ret;

	ret = s32_saradc_calibration(adc);
	s32_saradc_powerdown(adc->base);
	if (ret) {
		ERROR("SARADC calibration failed\n");
		return ret;
	}

	mcr = mmio_read_32(adc->base + SARADC_MCR) | SARADC_MCR_ADCLKSE;
	mmio_write_32(adc->base + SARADC_MCR, mcr);

	mmio_write_32(adc->base + SARADC_CTR0, SARADC_CTR0_INPSAMP(0xFF));

	return 0;
}

int s32_adc_get_setup_from_fdt(void *fdt, int node,
			       struct s32_adc *adc)
{
	int ret;

	ret = fdt_get_reg_props_by_index(fdt, node, 0, &adc->base, NULL);
	if (ret) {
		ERROR("Invalid SARADC base address\n");
		return -EINVAL;
	}

	adc->num_channels = 8;

	return 0;
}
