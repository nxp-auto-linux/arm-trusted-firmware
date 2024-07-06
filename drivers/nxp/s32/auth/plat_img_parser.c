/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <drivers/auth/img_parser_mod.h>

#define LIB_NAME	"HSE Key Handles"

static void init(void)
{

}

static int check_integrity(void *img, unsigned int img_len)
{
	return IMG_PARSER_OK;
}

static int get_auth_param(const auth_param_type_desc_t *type_desc, void *img,
		   unsigned int img_len, void **param, unsigned int *param_len)
{
	return IMG_PARSER_OK;
}

REGISTER_IMG_PARSER_LIB(IMG_PLAT, LIB_NAME, init,
			check_integrity, get_auth_param);
