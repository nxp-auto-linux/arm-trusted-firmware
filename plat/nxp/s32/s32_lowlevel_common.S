/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <asm_macros.S>
#include <console_macros.S>
#include <lib/utils_def.h>
#include <plat_macros.S>
#include <platform_def.h>

.globl plat_is_my_cpu_primary
.globl plat_my_core_pos
.globl plat_core_pos_by_mpidr
.globl plat_panic_handler
.globl s32_ncore_isol_cluster0
.globl reset_registers_for_lockstep
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

/* Set the CAIUTC[IsolEn] bit for the primary A53 cluster.
 * This is so cache invalidate operations from the early TF-A boot code
 * won't cause Ncore to crash.
 *
 * Clobber list: x8,x9,x10
 */
func s32_ncore_isol_cluster0
	movz	x8, #S32_NCORE_CAIU0_BASE_ADDR_H, lsl #16
	ldr	x9, [x8, #NCORE_CAIUTC_OFF]
	movz	x10, #1
	lsl	x10, x10, #NCORE_CAIUTC_ISOLEN_SHIFT
	orr	x9, x9, x10
	str	x9, [x8, #NCORE_CAIUTC_OFF]

	ret
endfunc s32_ncore_isol_cluster0

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

/* Clobber list: x0,x24,x25,x26,27,x28 */
func reset_registers_for_lockstep
	/*
	 * Timers reset must be done when lockstep is enabled to avoid RCCU
	 * mismatch errors. Reset should be executed as early as possible
	 * before any read access to these counters. Resetting them for all boot
	 * flows assures consistent values
	 * This must be done in EL3 and executed for all cores.
	 */

	mov x0, #0x0
	msr cntkctl_el1, x0

	msr cntp_tval_el0, x0
	msr cntp_ctl_el0, x0
	msr cntp_cval_el0, x0

	msr cntv_tval_el0, x0
	msr cntv_cval_el0, x0
	msr cntv_ctl_el0, x0

	msr cntvoff_el2, x0
	msr cnthctl_el2, x0

	msr cnthp_tval_el2, x0
	msr cnthp_ctl_el2, x0
	msr cnthp_cval_el2, x0

	msr cntps_tval_el1, x0
	msr cntps_ctl_el1, x0
	msr cntps_cval_el1, x0

	/*
	 * Lockstep sync GPR registers: write x19-x28 callee-saved registers
	 * as defined in procedure call standard for the ARM 64-bit. These
	 * registers may be saved to stack without being initialized, setting
	 * them is needed to avoid lockstep errors.
	 * x19-x23 are already initialized up to this point
	 */

	mov x24, #0
	mov x25, #0
	mov x26, #0
	mov x27, #0
	mov x28, #0

	ret
endfunc reset_registers_for_lockstep
