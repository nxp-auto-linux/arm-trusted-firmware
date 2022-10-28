// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022 NXP
 */

#ifndef S32_ADC_H
#define S32_ADC_H

struct s32_adc {
	unsigned long base;
	uint8_t num_channels;
};

int s32_adc_get_setup_from_fdt(void *fdt, int node,
			       struct s32_adc *adc);
int s32_adc_read_channel(struct s32_adc *adc, uint8_t channel,
			 unsigned int *data);
int s32_adc_init(struct s32_adc *adc);

#endif
