/*
 * Copyright 2020, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef BL31_SRAM_H
#define BL31_SRAM_H

/* The content of BL31_SRAM stage */
extern unsigned char bl31sram[];
extern unsigned int bl31sram_len;
extern unsigned int bl31sram_ro_start;
extern unsigned int bl31sram_ro_end;

typedef void (*bl31_sram_entry_t)(void);

#endif
