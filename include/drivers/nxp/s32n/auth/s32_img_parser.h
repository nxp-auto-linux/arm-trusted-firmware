/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * NXP S32 Platform Image Parser - platform hooks (S32N)
 *
 * Copyright 2026 NXP
 */

#ifndef S32_IMG_PARSER_H
#define S32_IMG_PARSER_H

#include <hse_interface.h>

/**
 * plat_img_parser_push_key() - push a verified key to the CAAM Secure Memory
 * @key_handle: handle of the key in the HSE2 key catalog
 * @key_info: key information, as reported by HSE2 for @key_handle
 *
 * Push the (public) key from the HSE2 keystore to the CAAM Secure Memory
 * window owned by the Job Ring used for CAAM-accelerated RSA public key
 * operations. Meant to be called by the image parser, after the key handle
 * has been successfully verified.
 *
 * Return: 0 on success, negative value on error.
 */
int plat_img_parser_push_key(hseKeyHandle_t key_handle,
			     const hseKeyInfo_t *key_info);

#endif /* S32_IMG_PARSER_H */
