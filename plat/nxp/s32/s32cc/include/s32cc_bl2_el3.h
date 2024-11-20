/*
 * Copyright 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32CC_BL2_EL3_H
#define S32CC_BL2_EL3_H

#include <common/desc_image_load.h>
#include "s32cc_mc_rgm.h"

struct s32_mmu_filter {
	const uintptr_t *base_addrs;
	size_t n_base_addrs;
};

int add_bl31_img_to_mem_params_descs(bl_mem_params_node_t *params,
				     size_t *index, size_t size);
int add_bl32_img_to_mem_params_descs(bl_mem_params_node_t *params,
				     size_t *index, size_t size);
int add_bl32_extra1_img_to_mem_params_descs(bl_mem_params_node_t *params,
					    size_t *index, size_t size);
int add_bl33_img_to_mem_params_descs(bl_mem_params_node_t *params,
				     size_t *index, size_t size);

int s32_el3_mmu_ddr_fixup(void);
int s32_el3_mmu_fixup(const struct s32_mmu_filter *filters, size_t n_filters);
void clear_swt_faults(void);
void clear_reset_cause(void);
const char *get_reset_cause_str(enum reset_cause reset_cause);

#endif /* S32CC_BL2_EL3_H */
