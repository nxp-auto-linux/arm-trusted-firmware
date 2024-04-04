// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2023-2024 NXP
 */

#include <stdint.h>
#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <drivers/nxp/s32/stm/s32_stm.h>
#include <lib/libc/errno.h>
#include <lib/mmio.h>
#include <libfdt.h>
#include <s32cc_dt.h>

#define STM_CR				0x00
#define STM_CNT				0x04

#define STM_CR_TEN			BIT_32(0)
#define STM_CR_FRZ			BIT_32(1)
#define STM_CR_CPS_MASK     GENMASK(15, 8)
#define STM_CR_CPS_OFFSET   (8u)
#define STM_CR_CPS(X)       ((X) << STM_CR_CPS_OFFSET)
#define STM_CNT_VAL         (0)

static int s32_stm_get_setup_from_fdt(void *fdt, int node, struct s32_stm *stm)
{
	int ret;

	ret = fdt_get_reg_props_by_index(fdt, node, 0, &stm->base, NULL);
	if (ret) {
		ERROR("Invalid STM base address\n");
		return -EINVAL;
	}

	return 0;
}

int s32_stm_init(struct s32_stm *stm)
{
	void *fdt = NULL;
	int stm_node;
	int ret;

	if (dt_open_and_check() < 0)
		return -EINVAL;

	if (fdt_get_address(&fdt) == 0)
		return -EINVAL;

	stm_node = fdt_node_offset_by_compatible(fdt, -1, "nxp,s32cc-stm-global");
	if (stm_node == -1)
		return -ENODEV;

	ret = s32_stm_get_setup_from_fdt(fdt, stm_node, stm);
	if (ret)
		return ret;

	return 0;
}

bool s32_stm_is_enabled(struct s32_stm *stm)
{
	uint32_t cr = mmio_read_32(stm->base + STM_CR);

	return !!(cr & STM_CR_TEN);
}

void s32_stm_enable(struct s32_stm *stm, bool enable)
{
	uint32_t cr = 0;

	if (enable) {
		mmio_write_32(stm->base + STM_CNT, STM_CNT_VAL);
		cr |= STM_CR_FRZ;
		cr |= STM_CR_TEN;
		cr |= STM_CR_CPS(0) & STM_CR_CPS_MASK;
		mmio_write_32(stm->base + STM_CR, cr);
	} else {
		cr = mmio_read_32(stm->base + STM_CR);
		cr &= ~STM_CR_TEN;
		mmio_write_32(stm->base + STM_CR, cr);
	}
}

uint32_t s32_stm_get_count(struct s32_stm *stm)
{
	return mmio_read_32(stm->base + STM_CNT);
}
