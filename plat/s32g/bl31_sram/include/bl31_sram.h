/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef BL31_SRAM_H
#define BL31_SRAM_H

/* The content of BL31_SRAM stage */
extern unsigned char bl31sram[];
extern unsigned int bl31sram_len;

typedef void (*bl31_sram_entry_t)(void);

#endif
