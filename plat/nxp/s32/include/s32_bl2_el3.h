/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32_BL2_EL3_H
#define S32_BL2_EL3_H

#include <common/desc_image_load.h>

void add_fip_img_to_mem_params_descs(bl_mem_params_node_t *params, size_t *index);
void add_bl31_img_to_mem_params_descs(bl_mem_params_node_t *params, size_t *index);
void add_bl32_img_to_mem_params_descs(bl_mem_params_node_t *params, size_t *index);
void add_bl32_extra1_img_to_mem_params_descs(bl_mem_params_node_t *params, size_t *index);
void add_bl33_img_to_mem_params_descs(bl_mem_params_node_t *params, size_t *index);
void add_invalid_img_to_mem_params_descs(bl_mem_params_node_t *params, size_t *index);

#endif /* S32_BL2_EL3_H */
