/*
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32_BL2_EL3_H
#define S32_BL2_EL3_H

#include <common/desc_image_load.h>
#include "s32_mc_rgm.h"

void add_fip_img_to_mem_params_descs(bl_mem_params_node_t *params, size_t *index);
void add_bl31_img_to_mem_params_descs(bl_mem_params_node_t *params, size_t *index);
void add_bl32_img_to_mem_params_descs(bl_mem_params_node_t *params, size_t *index);
void add_bl32_extra1_img_to_mem_params_descs(bl_mem_params_node_t *params, size_t *index);
void add_bl33_img_to_mem_params_descs(bl_mem_params_node_t *params, size_t *index);
void add_invalid_img_to_mem_params_descs(bl_mem_params_node_t *params, size_t *index);

int s32_el3_mmu_fixup(void);
void clear_swt_faults(void);
void clear_reset_cause(void);
const char *get_reset_cause_str(enum reset_cause reset_cause);

#endif /* S32_BL2_EL3_H */
