/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * NXP S32 Platform Image Parser - platform hooks (S32G/S32R)
 *
 * Copyright 2026 NXP
 */

#ifndef S32_IMG_PARSER_H
#define S32_IMG_PARSER_H

#include <hse_interface.h>

/**
 * plat_img_parser_push_key() - push a verified key to the CAAM Secure Memory
 * @key_handle: handle of the key in the HSE key catalog
 * @key_info: key information, as reported by HSE for @key_handle
 *
 * Nothing to do on this platform: keys are not pushed from the HSE keystore
 * to the CAAM. The function exists only to allow the image parser to call
 * it unconditionally.
 *
 * Return: 0 (always)
 */
static inline int plat_img_parser_push_key(hseKeyHandle_t key_handle,
					   const hseKeyInfo_t *key_info)
{
	return 0;
}

#endif /* S32_IMG_PARSER_H */
