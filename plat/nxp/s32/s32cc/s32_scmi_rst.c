// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021-2023 NXP */
#include <cdefs.h>
#include <clk/s32gen1_scmi_rst.h>
#include <common/debug.h>
#include <drivers/scmi-msg.h>
#include <drivers/scmi.h>
#include <dt-bindings/clock/s32gen1-clock.h>
#ifdef PLAT_s32r
#include <dt-bindings/reset/s32r45-scmi-reset.h>
#endif
#ifdef PLAT_s32g2
#include <dt-bindings/reset/s32g-scmi-reset.h>
#endif
#ifdef PLAT_s32g3
#include <dt-bindings/reset/s32g3-scmi-reset.h>
#endif

struct reset_entry {
	const char *name;
	uint32_t id;
	bool part;
	uint32_t mux_clock;
};

#define PART_RESET(ID, NAME) \
{ .part = true, .id = (ID), .name = (NAME), .mux_clock = S32GEN1_NO_MUX_ATTACHED, }

#define PERIPH_RESET(ID, NAME) \
{ .part = false, .id = (ID), .name = NAME, .mux_clock = S32GEN1_NO_MUX_ATTACHED, }

#define PERIPH_RESET_WMUX(ID, NAME, MUX_CLK) \
{ .part = false, .id = (ID), .name = NAME, .mux_clock = (MUX_CLK), }

static const struct reset_entry reset_table[] = {
	/* Partitions */
	[S32CC_SCMI_RST_PART0] = PART_RESET(0, "partition0"),
	[S32CC_SCMI_RST_PART1] = PART_RESET(1, "partition1"),
	[S32CC_SCMI_RST_PART2] = PART_RESET(2, "partition2"),
	[S32CC_SCMI_RST_PART3] = PART_RESET(3, "partition3"),
	/* Peripherals. See Reset chapter from RM */
	[S32CC_SCMI_RST_CM7_0] = PERIPH_RESET(0, "cm7_0"),
	[S32CC_SCMI_RST_CM7_1] = PERIPH_RESET(1, "cm7_1"),
	[S32CC_SCMI_RST_CM7_2] = PERIPH_RESET(2, "cm7_2"),
	[S32CC_SCMI_RST_DDR] = PERIPH_RESET_WMUX(3, "ddr",
						 S32GEN1_CLK_MC_CGM5_MUX0),
	[S32CC_SCMI_RST_PCIE0] = PERIPH_RESET(4, "pcie0"),
	[S32CC_SCMI_RST_SERDES0] = PERIPH_RESET(5, "serdes0"),
	[S32CC_SCMI_RST_PCIE1] = PERIPH_RESET(16, "pcie1"),
	[S32CC_SCMI_RST_SERDES1] = PERIPH_RESET(17, "serdes1"),
#ifdef PLAT_s32g2
	[S32CC_SCMI_RST_A53_0] = PERIPH_RESET(65, "a53_0"),
	[S32CC_SCMI_RST_A53_1] = PERIPH_RESET(66, "a53_1"),
	[S32CC_SCMI_RST_A53_2] = PERIPH_RESET(67, "a53_2"),
	[S32CC_SCMI_RST_A53_3] = PERIPH_RESET(68, "a53_3"),
	/* PFE and LLCE cannot be reset as an independent peripherals */
	[S32G_SCMI_RST_PFE] = PART_RESET(2, "pfe"),
	[S32G_SCMI_RST_LLCE] = PART_RESET(3, "llce"),
#endif
#ifdef PLAT_s32g3
	[S32CC_SCMI_RST_A53_0] = PERIPH_RESET(65, "a53_0"),
	[S32CC_SCMI_RST_A53_1] = PERIPH_RESET(66, "a53_1"),
	[S32CC_SCMI_RST_A53_2] = PERIPH_RESET(69, "a53_2"),
	[S32CC_SCMI_RST_A53_3] = PERIPH_RESET(70, "a53_3"),
	[S32G3_SCMI_RST_A53_4] = PERIPH_RESET(67, "a53_4"),
	[S32G3_SCMI_RST_A53_5] = PERIPH_RESET(68, "a53_5"),
	[S32G3_SCMI_RST_A53_6] = PERIPH_RESET(71, "a53_6"),
	[S32G3_SCMI_RST_A53_7] = PERIPH_RESET(72, "a53_7"),
	[S32G3_SCMI_RST_CM7_3] = PERIPH_RESET(6, "cm7_3"),
	/* PFE and LLCE cannot be reset as an independent peripherals */
	[S32G_SCMI_RST_PFE] = PART_RESET(2, "pfe"),
	[S32G_SCMI_RST_LLCE] = PART_RESET(3, "llce"),
#endif
#ifdef PLAT_s32r
	[S32CC_SCMI_RST_A53_0] = PERIPH_RESET(65, "a53_0"),
	[S32CC_SCMI_RST_A53_1] = PERIPH_RESET(66, "a53_1"),
	[S32CC_SCMI_RST_A53_2] = PERIPH_RESET(67, "a53_2"),
	[S32CC_SCMI_RST_A53_3] = PERIPH_RESET(68, "a53_3"),
	/* LAX and RADAR cannot be reset as independent peripherals */
	[S32R45_SCMI_RST_LAX] = PART_RESET(2, "lax"),
	[S32R45_SCMI_RST_RADAR] = PART_RESET(3, "radar"),
#endif
};

uint32_t get_reset_block(uint32_t scmi_id)
{
	return reset_table[scmi_id].id;
}

uint32_t get_part_id(uint32_t scmi_id)
{
	return reset_table[scmi_id].id;
}

size_t plat_scmi_rstd_count(unsigned int agent_id __unused)
{
	return ARRAY_SIZE(reset_table);
}

const char *plat_scmi_rstd_get_name(unsigned int agent_id __unused,
				    unsigned int scmi_id)
{
	if (scmi_id >= ARRAY_SIZE(reset_table))
		return NULL;

	return reset_table[scmi_id].name;
}

int32_t plat_scmi_rstd_autonomous(unsigned int agent_id __unused,
				  unsigned int scmi_id __unused,
				  unsigned int state __unused)
{
	return SCMI_NOT_SUPPORTED;
}

static bool is_partition(unsigned int scmi_id)
{
	return reset_table[scmi_id].part;
}

static uint32_t get_mux_clk(unsigned int scmi_id)
{
	return reset_table[scmi_id].mux_clock;
}

int32_t plat_scmi_rstd_set_state(unsigned int agent_id __unused,
				 unsigned int scmi_id,
				 bool assert_not_deassert)
{
	int ret;

	if (scmi_id >= ARRAY_SIZE(reset_table))
		return SCMI_OUT_OF_RANGE;

	if (is_partition(scmi_id))
		ret = s32gen1_reset_partition(get_part_id(scmi_id),
					      assert_not_deassert);
	else
		ret = s32gen1_reset_periph(get_reset_block(scmi_id),
					   assert_not_deassert,
					   get_mux_clk(scmi_id));

	if (ret)
		return SCMI_HARDWARE_ERROR;

	return SCMI_SUCCESS;
}

