// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2021, 2023-2024 NXP
 */
#include <clk/clk.h>
#include <clk/mc_rgm_regs.h>
#include <clk/s32gen1_clk_funcs.h>
#include <clk/s32gen1_clk_modules.h>
#include <clk/s32gen1_scmi_clk.h>
#include <drivers/delay_timer.h>
#include <clk/s32gen1_scmi_rst.h>
#include <lib/mmio.h>
#include <inttypes.h>
#include <s32cc_mc_rgm.h>
#include <s32cc_mc_me.h>

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

static int get_rgm_reset_part(unsigned long id, uint32_t *rgm_part)
{

	/* MC_RGM valid reset IDs */
	switch (id) {
	case 0 ... 17:
		*rgm_part = 0;
		break;
	case 64 ... 68:
		*rgm_part = 1;
		break;
	case 128 ... 130:
		*rgm_part = 2;
		break;
	case 192 ... 194:
		*rgm_part = 3;
		break;
	default:
		ERROR("Wrong reset id: %lu\n", id);
		return -EINVAL;
	};

	return 0;
}

int s32gen1_assert_rgm(void *rgm, bool asserted, uint32_t id)
{
	uintptr_t pstat;
	uint32_t id_offset = id % 32;
	uint32_t prst_val, stat_mask = PSTAT_PERIPH_n_STAT(id_offset);
	uint32_t rgm_part;
	const char *msg;
	int ret;

	ret = get_rgm_reset_part(id, &rgm_part);
	if (ret)
		return ret;

	pstat = RGM_PSTAT(rgm, rgm_part);

	prst_val = s32_mc_rgm_read(rgm, rgm_part);
	if (asserted) {
		msg = "assert";
		prst_val |= PRST_PERIPH_n_RST(id_offset);
	} else {
		msg = "deassert";
		prst_val &= ~PRST_PERIPH_n_RST(id_offset);
	}

	s32_mc_rgm_periph_reset(rgm, rgm_part, prst_val);
	spin_until_cond(in_reset(pstat, stat_mask, asserted),
			S32GEN1_RESET_TIMEOUT_US);
	if (asserted) {
		if (mmio_read_32(pstat) & stat_mask)
			return 0;
	} else {
		if (!(mmio_read_32(pstat) & stat_mask))
			return 0;
	}

	ERROR("Failed to %s reset for id %" PRIu32 "\n", msg, id);
	return -EINVAL;
}

static struct clk_driver *get_clk_drv(void)
{
	static struct clk_driver *drv;

	if (!drv)
		drv = get_clk_driver_by_name(S32GEN1_CLK_DRV_NAME);

	return drv;
}

int s32gen1_reset_periph(uint32_t periph_id, bool assert, uint32_t mux_clk)
{
	struct clk_driver *drv = get_clk_drv();
	struct s32gen1_clk_priv *priv = NULL;
	struct s32gen1_clk *clk;
	struct s32gen1_mux *mux = NULL;
	bool restore_cgm_mux = false;
	int ret, err;

	if (!drv) {
		ERROR("Failed to get a valid reference for the clock driver\n");
		return -EIO;
	}

	priv = get_clk_drv_data(drv);
	if (!priv) {
		ERROR("Failed to get the clock driver private data\n");
		return -EIO;
	}

	if (mux_clk != S32GEN1_NO_MUX_ATTACHED) {
		clk = get_clock(mux_clk);
		if (!clk) {
			ERROR("Clock %" PRIu32 " is not part of the clock tree\n",
			      mux_clk);
			return -EIO;
		}

		if (!is_mux(clk)) {
			ERROR("Clock ID %" PRIu32 " is not a mux\n", mux_clk);
			return -EIO;
		}

		mux = clk2mux(clk);
		if (!mux)
			return -EINVAL;

		ret = s32gen1_cgm_mux_to_safe(mux, priv);
		if (ret) {
			ERROR("Safe clock transition has failed for mux %" PRIu32 "\n",
			      mux_clk);
			return ret;
		}

		restore_cgm_mux = true;
	}

	ret = s32gen1_assert_rgm(priv->rgm, assert, periph_id);
	if (ret) {
		ERROR("The reset of %" PRIu32 " periph has failed\n",
		      periph_id);
	}

	if (restore_cgm_mux) {
		err = s32gen1_enable_cgm_mux(mux, priv, true);
		if (!ret && err)
			ret = err;
	}

	return ret;
}

int s32gen1_reset_partition(unsigned int part_id, bool assert_not_deassert)
{
	if (assert_not_deassert)
		return mc_me_disable_partition(part_id);

	mc_me_enable_partition(part_id);

	return 0;
}

