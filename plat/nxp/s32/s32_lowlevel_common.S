/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <asm_macros.S>
#include <platform_def.h>
#include <plat_macros.S>

.globl plat_is_my_cpu_primary
.globl plat_my_core_pos
.globl plat_core_pos_by_mpidr
.globl plat_panic_handler
.globl s32_plat_data_stack
.globl s32_crash_reg_stash

/**
 * Use a local buffer as stack for Linflex crash callbacks and SRAM
 * initialization
 */
.section .data.s32_plat_data_stack
	.balign 16
	s32_plat_data_stack: .skip S32_CRASH_STACK_SIZE

.section .data.s32_crash_reg_stash
	.align 3
	s32_crash_reg_stash: .quad 0, 0, 0, 0

func plat_panic_handler
	wfi
	b	plat_panic_handler
endfunc plat_panic_handler

/* Clobber list: x0,x1,x7,x8
 */
func plat_is_my_cpu_primary
	mov	x7, x30
	bl	plat_my_core_pos
	cmp	x0, #S32_PLAT_PRIMARY_CPU
	cset	x0, eq
	mov	x30, x7
	ret
endfunc plat_is_my_cpu_primary

/* Out: x0
 * Clobber list: x0,x1,x8
 */
func plat_my_core_pos
	mov	x8, x30
	mrs x0, mpidr_el1
	bl	s32_core_pos_by_mpidr
	mov	x30, x8
	ret
endfunc plat_my_core_pos

/* In:	x0 -  MPIDR_EL1
 * Out:	x0
 * Clobber list: x0,x1
 */
func s32_core_pos_by_mpidr
	and	x0, x0, #S32_MPIDR_CPU_CLUSTER_MASK
	and	x1, x0, #S32_MPIDR_CPU_MASK
	lsr	x0, x0, #S32_MPIDR_CLUSTER_SHIFT
	add	x0, x1, x0, lsl #S32_MPIDR_CPU_MASK_BITS
	ret
endfunc s32_core_pos_by_mpidr

/* Clobber list: x7 */
func plat_core_pos_by_mpidr
	mov	x7, x30
	bl	s32_core_pos_by_mpidr
	mov	x30, x7
	ret
endfunc plat_core_pos_by_mpidr
