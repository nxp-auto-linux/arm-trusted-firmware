/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <common/debug.h>
#include <clk/s32gen1_scmi_rst.h>
#include <lib/mmio.h>
#include <s32_bl_common.h>
#include <s32cc_bl_common.h>
#include <s32cc_flexnoc.h>
#include <s32cc_mc_me.h>

#define NOC_0_CONFIG_BASE	0x50000000u
#define NOC_1_CONFIG_BASE	0x47000000u
#define NOC_CONFIG_AREA_SIZE	0x0007ffffu

#define CCTI_FAULT_CTRL_BASE	0x50500000u

#define CAIU0_QOS_GEN_BASE	0x50500100u
#define CAIU1_QOS_GEN_BASE	0x50500200u
#define CAIU_QOS_SIZE		0x0000001cu
#define CAIU_QOS_GEN_AREA_SIZE ((CAIU1_QOS_GEN_BASE + CAIU_QOS_SIZE) - CCTI_FAULT_CTRL_BASE)

typedef struct {
	uint32_t addr;
	uint32_t mask;
	uint32_t value;
} flex_noc_config_t;

static const flex_noc_config_t flex_noc_part1_configs[] = {
	{0x50500108u, 0x00000303u, 0x00000002u},
	{0x50500208u, 0x00000303u, 0x00000002u},
	{0x50000408u, 0x00000707u, 0x00000504u},
	{0x50000410u, 0x00001fffu, 0x000000c5u},
	{0x50000488u, 0x00000707u, 0x00000503u},
	{0x50000490u, 0x00001fffu, 0x000000c5u},
	{0x50000508u, 0x00000707u, 0x00000501u},
	{0x50000510u, 0x00001fffu, 0x000003d8u},
	{0x50000518u, 0x00000001u, 0x00000001u},
	{0x50018488u, 0x00000707u, 0x00000505u},
	{0x50018490u, 0x00000fffu, 0x00000014u},
};

static const flex_noc_config_t flex_noc_part2_configs[] = {
	{0x50000708u, 0x00000707u, 0x00000606u},
	{0x50000788u, 0x00000707u, 0x00000606u},
	{0x50000808u, 0x00000707u, 0x00000503u},
	{0x50000888u, 0x00000707u, 0x00000707u},
	{0x50000890u, 0x00000fffu, 0x00000026u},
	{0x50000388u, 0x00000707u, 0x00000505u},
	{0x50000908u, 0x00000707u, 0x00000606u},
	{0x50000988u, 0x00000707u, 0x00000501u},
	{0x50000990u, 0x00001fffu, 0x00000280u},
	{0x50020088u, 0x00000707u, 0x00000504u},
	{0x50020090u, 0x00000fffu, 0x00000040u},
	{0x50021088u, 0x00000707u, 0x00000504u},
	{0x50021090u, 0x00000fffu, 0x00000040u},
	{0x50022088u, 0x00000707u, 0x00000504u},
	{0x50022090u, 0x00000fffu, 0x00000040u},
	{0x50023088u, 0x00000707u, 0x00000504u},
	{0x50023090u, 0x00000fffu, 0x00000040u},
	{0x47001508u, 0x00000707u, 0x00000504u},
	{0x47001510u, 0x00000fffu, 0x00000100u},
	{0x47000088u, 0x00000707u, 0x00000504u},
	{0x47000090u, 0x00000fffu, 0x00000100u},
	{0x47000108u, 0x00000707u, 0x00000501u},
	{0x47000110u, 0x00000fffu, 0x00000080u},
	{0x47000188u, 0x00000707u, 0x00000501u},
	{0x47000190u, 0x00000fffu, 0x00000080u},
	{0x47000208u, 0x00000707u, 0x00000501u},
	{0x47000210u, 0x00000fffu, 0x00000015u},
	{0x47000288u, 0x00000707u, 0x00000501u},
	{0x47000290u, 0x00000fffu, 0x00000015u},
	{0x47000308u, 0x00000707u, 0x00000501u},
	{0x4700030cu, 0x00000003u, 0x00000000u},
	{0x47000310u, 0x00000fffu, 0x0000002au},
	{0x47000314u, 0x000003ffu, 0x00000040u},
	{0x47000388u, 0x00000707u, 0x00000505u},
	{0x4700038cu, 0x00000003u, 0x00000000u},
};

static const flex_noc_config_t flex_noc_part3_configs[] = {
	{0x50018408u, 0x00000707u, 0x00000707u},
};

static const mmap_region_t noc_dyn_regs[] = {
	MAP_REGION_FLAT(NOC_0_CONFIG_BASE, MMU_ROUND_UP_TO_PAGE(NOC_CONFIG_AREA_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(NOC_1_CONFIG_BASE, MMU_ROUND_UP_TO_PAGE(NOC_CONFIG_AREA_SIZE),
			MT_DEVICE | MT_RW),
	MAP_REGION_FLAT(CCTI_FAULT_CTRL_BASE, MMU_ROUND_UP_TO_PAGE(CAIU_QOS_GEN_AREA_SIZE),
			MT_DEVICE | MT_RW),
};

typedef struct {
	const flex_noc_config_t *p_cfg;
	uint32_t configs_nb;
} flex_noc_part_config_t;

static const flex_noc_part_config_t part_configs[] = {
	{},
	{
		.p_cfg = flex_noc_part1_configs,
		.configs_nb = ARRAY_SIZE(flex_noc_part1_configs),
	},
	{
		.p_cfg = flex_noc_part2_configs,
		.configs_nb = ARRAY_SIZE(flex_noc_part2_configs),
	},
	{
		.p_cfg = flex_noc_part3_configs,
		.configs_nb = ARRAY_SIZE(flex_noc_part3_configs),
	},
};

static int mmap_noc_regions(void)
{
	const size_t dyn_regs_nb = ARRAY_SIZE(noc_dyn_regs);
	static bool mmap_added;
	uint32_t idx;
	int ret;

	if (!is_mmu_el3_enabled() || mmap_added)
		return 0;

	for (idx = 0; idx < dyn_regs_nb; idx++) {
		const mmap_region_t *reg = &noc_dyn_regs[idx];

		ret = mmap_add_dynamic_region(reg->base_pa, reg->base_va, reg->size, reg->attr);
		if (ret) {
			ERROR("Failed to map NoC region 0x%llx dynamically. Error: %d\n",
			      reg->base_pa, ret);
			return ret;
		}
	}
	mmap_added = true;

	return 0;
}

static void apply_noc_part_settings(uint32_t part_nb)
{
	const uint32_t configs_nb = part_configs[part_nb].configs_nb;
	const flex_noc_part_config_t *p_part_cfg = &part_configs[part_nb];
	uint32_t current_cfg, idx;

	for (idx = 0; idx < configs_nb; idx++) {
		const flex_noc_config_t *cfg = &p_part_cfg->p_cfg[idx];

		current_cfg = mmio_read_32(cfg->addr) & cfg->mask;

		if (current_cfg != cfg->value)
			mmio_clrsetbits_32(cfg->addr, cfg->mask, cfg->value);
	}
}

int platform_adjust_noc_settings(void)
{
	uint32_t part_nb;
	int ret;

	ret = mmap_noc_regions();
	if (ret)
		return ret;

	for (part_nb = 1; part_nb < ARRAY_SIZE(part_configs); part_nb++) {
		if (s32_is_interconnect_disabled(part_nb))
			s32_enable_interconnect(part_nb);

		apply_noc_part_settings(part_nb);
	}

	return 0;
}
