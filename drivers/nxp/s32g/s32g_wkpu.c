// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/*
 * Copyright 2020 NXP
 */
#include <common/debug.h>
#include <dt-bindings/reset/s32gen1-wkpu.h>
#include <errno.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <libfdt.h>

#include "s32g_dt.h"
#include "s32gen1-wkpu.h"

#define WKPU_RTC_IRQ		BIT(31)
#define WKPU_INPUTS_MASK	(0x7FFFFF)

#define WKUP_PUS_PU_OVERRIDE	BIT(31)

#define WKPU_WBMSR		(0x10)
#define WKPU_WISR		(0x14)
#define WKPU_WISR_MASK		(0xFFFFFFFF)
#define WKPU_IRER		(0x18)
#define WKPU_WRER		(0x1C)
#define WKPU_WIREER		(0x28)
#define WKPU_WIFEER		(0x2C)
#define WKPU_WIFER		(0x30)
#define WKPU_WIFER_MASK		(0x7FFFFFFF)
#define WKPU_WIPDER		(0x34)
#define WKPU_WIPDER_MASK	(0x7FFFFFFF)

struct s32gen1_wkpu {
	struct dt_node_info dt_info;
	uintptr_t gpr; /* WKPU GPR base address */
	uint32_t irqs; /* Enabled IRQs */
	uint32_t edges; /* Rising / Falling edges */
	uint32_t pulls_en; /* Enabled pull resistors */
	uint32_t pullups; /* Pull-ups / Pull-downs */
};

static struct s32gen1_wkpu gwkpu;

void s32gen1_wkpu_reset(void)
{
	/* Clear all interrupts */
	mmio_write_32(gwkpu.dt_info.base + WKPU_WISR, WKPU_WISR_MASK);
}

static void init_wkpu(struct s32gen1_wkpu *wkpu)
{
	uint32_t rising, falling;

	rising = wkpu->edges & wkpu->irqs;
	falling = ~(wkpu->edges) & wkpu->irqs;

	/* Disable interrupts */
	mmio_write_32(wkpu->dt_info.base + WKPU_IRER, 0x0);
	mmio_write_32(wkpu->dt_info.base + WKPU_WRER, 0x0);

	/* Clear all interrupts */
	mmio_write_32(wkpu->dt_info.base + WKPU_WISR, WKPU_WISR_MASK);

	/* Short boot */
	mmio_write_32(wkpu->dt_info.base + WKPU_WBMSR, 0x0);

	/* IRQs edges */
	mmio_write_32(wkpu->dt_info.base + WKPU_WIREER, rising);
	mmio_write_32(wkpu->dt_info.base + WKPU_WIFEER, falling);

	/* Enable filters for all external inputs */
	mmio_write_32(wkpu->dt_info.base + WKPU_WIFER, WKPU_WIFER_MASK);

	/* Enable pull-up/pull-down resistors */
	mmio_write_32(wkpu->gpr, WKUP_PUS_PU_OVERRIDE | wkpu->pullups);
	mmio_write_32(wkpu->dt_info.base + WKPU_WIPDER,
		      wkpu->pulls_en & WKPU_WIPDER_MASK);

	/* Enable interrupts */
	mmio_write_32(wkpu->dt_info.base + WKPU_IRER, wkpu->irqs);
	mmio_write_32(wkpu->dt_info.base + WKPU_WRER, wkpu->irqs);
}

static int init_from_dt(void *fdt, int fdt_offset, struct s32gen1_wkpu *wkpu)
{
	const fdt32_t *irq_ptr, *reg;
	uint32_t irq_num;
	uint32_t pull;
	int len;
	int irqs;
	int i;

	/* Register active nodes only */
	dt_fill_device_info(&wkpu->dt_info, fdt_offset);
	if (wkpu->dt_info.status != DT_ENABLED)
		return -1;

	reg = fdt_getprop(fdt, fdt_offset, "reg", &len);
	/* WKPU & GPR ranges */
	if (len < 4 * sizeof(uint32_t)) {
		ERROR("Missing GPR registers\n");
		return -EIO;
	}

	/* GPR Base address */
	wkpu->gpr = fdt32_to_cpu(reg[2]);

	irq_ptr = fdt_getprop(fdt, fdt_offset, "nxp,irqs", &len);
	if (!irq_ptr) {
		ERROR("\"nxp,irqs\" property is mandatory\n");
		return -EIO;
	}

	wkpu->irqs = 0;
	wkpu->edges = 0;
	wkpu->pulls_en = 0;
	wkpu->pullups = 0;

	/* Number of tuples : (IRQ, Edge, pull-up) */
	irqs = len / sizeof(uint32_t) / 3;
	for (i = 0; i < irqs; i++) {
		irq_num = fdt32_to_cpu(irq_ptr[i * 3]);
		wkpu->irqs |= BIT(irq_num);

		if (fdt32_to_cpu(irq_ptr[i * 3 + 1]) == S32GEN1_WKPU_IRQ_RISING)
			wkpu->edges |= BIT(irq_num);

		pull = fdt32_to_cpu(irq_ptr[i * 3 + 2]);
		if (pull != S32GEN1_WKPU_PULL_DIS) {
			wkpu->pulls_en |= BIT(irq_num);

			if (pull == S32GEN1_WKPU_PULL_UP)
				wkpu->pullups |= BIT(irq_num);
		}
	}

	wkpu->irqs &= (WKPU_INPUTS_MASK | WKPU_RTC_IRQ);
	wkpu->edges &= (WKPU_INPUTS_MASK | WKPU_RTC_IRQ);
	wkpu->pulls_en &= WKPU_INPUTS_MASK;
	wkpu->pullups &= WKPU_INPUTS_MASK;

	return 0;
}

int s32gen1_wkpu_init(void *fdt, int fdt_offset)
{
	int ret;

	ret = init_from_dt(fdt, fdt_offset, &gwkpu);
	if (ret)
		return ret;

	init_wkpu(&gwkpu);

	return 0;
}
