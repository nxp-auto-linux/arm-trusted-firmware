/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/desc_image_load.h>
#include <plat/common/platform.h>

static bl_mem_params_node_t bl2_mem_params_descs[] = {
	{
		.image_id = BL31_IMAGE_ID,
	},
	{
		.image_id = BL33_IMAGE_ID,
	},
};

REGISTER_BL_IMAGE_DESCS(bl2_mem_params_descs);
