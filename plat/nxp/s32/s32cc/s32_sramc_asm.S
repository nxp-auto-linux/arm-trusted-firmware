/*
 * Copyright 2019-2021, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <plat_macros.S>
#include "s32cc_sramc.h"

.globl _s32_sram_clr

/**
 * x0: start address of memory area to clear
 * x1: end address of memory area to clear
 * x0: return 0 on error or size of memory cleared on success
 *
 * Clobber list: x0,x1,x9,x10
 */
func _s32_sram_clr
	s32_save_regs
	s32_init_local_stack

	bl s32_sram_clear

	s32_check_stack_guard
	s32_restore_regs
	ret
endfunc _s32_sram_clr
