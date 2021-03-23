// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021 NXP
 */
#include <clk/clk.h>
#include <clk/mc_rgm_regs.h>
#include <clk/s32gen1_clk_modules.h>
#include <clk/s32gen1_scmi_clk.h>
#include <lib/mmio.h>
#include <drivers/delay_timer.h>

#define S32GEN1_RESET_TIMEOUT_US	(1000)

#define spin_until_cond(COND, TIMEOUT_US)		\
do {							\
	uint64_t _timeout = timeout_init_us(TIMEOUT_US);\
	do {						\
		if (COND)				\
			break;				\
	} while (!timeout_elapsed(_timeout));		\
} while (0)

static bool in_reset(uintptr_t pstat, uint32_t mask, bool asserted)
{
	bool res;

	if (asserted)
		res = !!(mmio_read_32(pstat) & mask);
	else
		res = !(mmio_read_32(pstat) & mask);

	return res;
}

static int get_reset_regs(unsigned long id, uintptr_t rgm,
			  uintptr_t *prst, uintptr_t *pstat)
{
	uint32_t rgm_set;

	/* MC_RGM valid reset IDs */
	switch (id) {
	case 0 ... 17:
		rgm_set = 0;
		break;
	case 64 ... 68:
		rgm_set = 1;
		break;
	case 128 ... 130:
		rgm_set = 2;
		break;
	case 192 ... 194:
		rgm_set = 3;
		break;
	default:
		ERROR("Wrong reset id: %lu\n", id);
		return -EINVAL;
	};

	*prst = RGM_PRST(rgm, rgm_set);
	*pstat = RGM_PSTAT(rgm, rgm_set);

	return 0;
}

static int s32gen1_assert_rgm(uintptr_t rgm, bool asserted, uint32_t id)
{
	uintptr_t prst, pstat;
	uint32_t id_offset = id % 32;
	uint32_t prst_val, stat_mask = PSTAT_PERIPH_n_STAT(id_offset);
	const char *msg;
	int ret;

	ret = get_reset_regs(id, rgm, &prst, &pstat);
	if (ret)
		return ret;

	prst_val = mmio_read_32(prst);
	if (asserted) {
		msg = "assert";
		prst_val |= PRST_PERIPH_n_RST(id_offset);
	} else {
		msg = "deassert";
		prst_val &= ~PRST_PERIPH_n_RST(id_offset);
	}

	mmio_write_32(prst, prst_val);
	spin_until_cond(in_reset(pstat, stat_mask, asserted),
			S32GEN1_RESET_TIMEOUT_US);
	if (asserted) {
		if (mmio_read_32(pstat) & stat_mask)
			return 0;
	} else {
		if (!(mmio_read_32(pstat) & stat_mask))
			return 0;
	}

	ERROR("Failed to %s reset for id %u\n", msg, id);
	return -EINVAL;
}

int s32gen1_reset_periph(uint32_t periph_id, bool assert)
{
	static struct clk_driver *drv;
	struct s32gen1_clk_priv *priv;

	if (!drv)
		drv = get_clk_driver_by_name(S32GEN1_CLK_DRV_NAME);

	priv = get_clk_drv_data(drv);

	return s32gen1_assert_rgm((uintptr_t)priv->rgm, assert, periph_id);
}
