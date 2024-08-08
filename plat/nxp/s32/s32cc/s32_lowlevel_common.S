/*
 * Copyright 2021, 2023-2024 NXP
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
.globl s32_core_pos_by_mpidr
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

/* void plat_crash_console_flush(void); */
func plat_crash_console_flush
	mov_imm	x0, S32_UART_BASE
	b	console_linflex_core_flush
	ret
endfunc plat_crash_console_flush

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
	and	x1, x0, #MPIDR_CPU_MASK
	and	x0, x0, #MPIDR_CLUSTER_MASK
	lsr	x0, x0, #MPIDR_AFF1_SHIFT
	add	x0, x1, x0, lsl #S32_MPIDR_CPU_MASK_BITS
	ret
endfunc s32_core_pos_by_mpidr

/* Clobber list: x0-x15, x17, x18, x20-x29 */
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
	 * Lockstep sync GPR registers
	 * This breaks Aarch64 Procedure Call Standard, but it should
	 * not matter given that we are after only a few instructions
	 * after core reset.
	 */
	mov x1, #0
	mov x2, #0
	mov x3, #0
	mov x4, #0
	mov x5, #0
	mov x6, #0
	mov x7, #0
	mov x8, #0
	mov x9, #0
	mov x10, #0
	mov x11, #0
	mov x12, #0
	mov x13, #0
	mov x14, #0
	mov x15, #0
	/* x16 contains a return address (x30 state) */
	mov x17, #0
	mov x18, #0
	/* x19 contains a return address (x30 state) */
	mov x20, #0
	mov x21, #0
	mov x22, #0
	mov x23, #0
	mov x24, #0
	mov x25, #0
	mov x26, #0
	mov x27, #0
	mov x28, #0
	mov x29, #0

	/* SIMD regs reset */
	movi d0, #0
	movi d1, #0
	movi d2, #0
	movi d3, #0
	movi d4, #0
	movi d5, #0
	movi d6, #0
	movi d7, #0
	movi d8, #0
	movi d9, #0
	movi d10, #0
	movi d11, #0
	movi d12, #0
	movi d13, #0
	movi d14, #0
	movi d15, #0
	movi d16, #0
	movi d17, #0
	movi d18, #0
	movi d19, #0
	movi d20, #0
	movi d21, #0
	movi d22, #0
	movi d23, #0
	movi d24, #0
	movi d25, #0
	movi d26, #0
	movi d27, #0
	movi d28, #0
	movi d29, #0
	movi d30, #0
	movi d31, #0

	ret
endfunc reset_registers_for_lockstep
