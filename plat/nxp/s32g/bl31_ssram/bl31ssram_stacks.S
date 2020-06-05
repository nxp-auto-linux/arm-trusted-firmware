/*
 * Copyright (c) 2014-2019, ARM Limited and Contributors. All rights reserved.
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <platform_def.h>


	.local	platform_normal_stacks
	.weak	bl31ssram_set_my_stack
	.weak	bl31ssram_get_my_stack

	/* -----------------------------------------------------
	 * uintptr_t bl31ssram_get_my_stack ()
	 *
	 * For cold-boot BL images, only the primary CPU needs a
	 * stack. This function returns the stack pointer for a
	 * stack allocated in device memory.
	 * -----------------------------------------------------
	 */
func bl31ssram_get_my_stack
	get_up_stack platform_normal_stacks, BL31SSRAM_STACK_SIZE
	ret
endfunc bl31ssram_get_my_stack

	/* -----------------------------------------------------
	 * void bl31ssram_set_my_stack ()
	 *
	 * For cold-boot BL images, only the primary CPU needs a
	 * stack. This function sets the stack pointer to a stack
	 * allocated in normal memory.
	 * -----------------------------------------------------
	 */
func bl31ssram_set_my_stack
	get_up_stack platform_normal_stacks, BL31SSRAM_STACK_SIZE
	mov sp, x0
	ret
endfunc bl31ssram_set_my_stack

	/* -----------------------------------------------------
	 * Single cpu stack in normal memory.
	 * Used for C code during boot, BL31SSRAM_STACK_SIZE bytes
	 * are allocated
	 * -----------------------------------------------------
	 */
declare_stack platform_normal_stacks, bl31ssram_normal_stacks, \
		BL31SSRAM_STACK_SIZE, 1, CACHE_WRITEBACK_GRANULE
