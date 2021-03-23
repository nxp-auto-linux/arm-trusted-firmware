// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021 NXP */
#include <cdefs.h>
#include <common/debug.h>
#include <drivers/st/scmi.h>
#include <drivers/st/scmi-msg.h>
#include <dt-bindings/reset/s32g-scmi-reset.h>
#include <clk/s32gen1_scmi_rst.h>

struct reset_entry {
	const char *name;
	uint32_t id;
	bool part;
};

#define PART_RESET(ID, NAME) \
{ .part = true, .id = (ID), .name = (NAME) }

#define PERIPH_RESET(ID, NAME) \
{ .part = false, .id = (ID), .name = NAME }

static const struct reset_entry reset_table[] = {
	/* Partitions */
	[S32GEN1_SCMI_RST_PART0] = PART_RESET(0, "partition0"),
	[S32GEN1_SCMI_RST_PART1] = PART_RESET(1, "partition1"),
	[S32GEN1_SCMI_RST_PART2] = PART_RESET(2, "partition2"),
	[S32GEN1_SCMI_RST_PART3] = PART_RESET(3, "partition3"),
	/* Peripherals. See Reset chapter from RM */
	[S32GEN1_SCMI_RST_CM7_0] = PERIPH_RESET(0, "cm7_0"),
	[S32GEN1_SCMI_RST_CM7_1] = PERIPH_RESET(1, "cm7_1"),
	[S32GEN1_SCMI_RST_CM7_2] = PERIPH_RESET(2, "cm7_2"),
	[S32GEN1_SCMI_RST_DDR] = PERIPH_RESET(3, "ddr"),
	[S32GEN1_SCMI_RST_PCIE0] = PERIPH_RESET(4, "pcie0"),
	[S32GEN1_SCMI_RST_SERDES0] = PERIPH_RESET(5, "serdes0"),
	[S32GEN1_SCMI_RST_PCIE1] = PERIPH_RESET(16, "pcie1"),
	[S32GEN1_SCMI_RST_SERDES1] = PERIPH_RESET(17, "serdes1"),
	[S32GEN1_SCMI_RST_A53_0] = PERIPH_RESET(65, "a53_0"),
	[S32GEN1_SCMI_RST_A53_1] = PERIPH_RESET(66, "a53_1"),
	[S32GEN1_SCMI_RST_A53_2] = PERIPH_RESET(67, "a53_2"),
	[S32GEN1_SCMI_RST_A53_3] = PERIPH_RESET(68, "a53_3"),
	[S32G_SCMI_RST_PFE] = PERIPH_RESET(128, "pfe"),
	[S32G_SCMI_RST_LLCE] = PERIPH_RESET(192, "llce"),
};

uint32_t get_reset_block(uint32_t scmi_id)
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

int32_t plat_scmi_rstd_set_state(unsigned int agent_id __unused,
				 unsigned int scmi_id,
				 bool assert_not_deassert)
{
	int ret;

	if (scmi_id >= ARRAY_SIZE(reset_table))
		return SCMI_OUT_OF_RANGE;

	ret = s32gen1_reset_periph(get_reset_block(scmi_id),
				   assert_not_deassert);
	if (ret)
		return SCMI_HARDWARE_ERROR;

	return SCMI_SUCCESS;
}

