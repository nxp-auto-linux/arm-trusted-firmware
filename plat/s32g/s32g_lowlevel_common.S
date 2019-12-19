/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <console_macros.S>
#include "platform_def.h"

.globl plat_is_my_cpu_primary
.globl plat_my_core_pos
.globl plat_core_pos_by_mpidr
.globl s32g_sram_init


/* Set the CAIUTC[IsolEn] bit for the primary A53 cluster.
 * This is so cache invalidate operations from the early TF-A boot code
 * won't cause Ncore to crash.
 *
 * Clobber list: x8,x9,x10
 */
func s32g_ncore_isol_cluster0
	movz	x8, #S32G_NCORE_CAIU0_BASE_ADDR_H, lsl #16
	ldr	x9, [x8, #NCORE_CAIUTC_OFF]
	movz	x10, #1
	lsl	x10, x10, #NCORE_CAIUTC_ISOLEN_SHIFT
	orr	x9, x9, x10
	str	x9, [x8, #NCORE_CAIUTC_OFF]

	ret
endfunc s32g_ncore_isol_cluster0


/* Quick and dirty (read: 'naive') implementation of U-Boot's dma_mem_clr
 * routine. TODO: We will switch to that one, based on eDMA, as soon as we
 * finish up core clock debugging. Meanwhile, preserve the calling convention.
 *
 * x0: start address of memory to clear
 * x1: size of memory area to clear
 *
 * Clobber list: x0,x1,x9,x11
 *
 * NOTE/FIXME: Memory area MUST be 8-byte aligned. We are not making any checks.
 */
func s32g_sram_init
	mov	x9, x30

	add	x1, x0, x1
init:
	str	xzr, [x0]
	ldr	x11, [x0], #8
	cmp	x11, xzr
	bne	assert_loop
	cmp	x0, x1
	blt	init

	mov x30, x9
	ret
assert_loop:
	b	.
	no_ret	plat_panic_handler
endfunc s32g_sram_init


/* Clobber list: x0,x1,x7,x8
 */
func plat_is_my_cpu_primary
	mov	x7, x30
	bl	plat_my_core_pos
	cmp	x0, #S32G_PLAT_PRIMARY_CPU
	cset	x0, eq
	mov	x30, x7
	ret
endfunc plat_is_my_cpu_primary


/* Out: x0
 * Clobber list: x0,x1,x8
 */
func plat_my_core_pos
	mov	x8, x30
	mrs 	x0, mpidr_el1
	bl	s32g_core_pos_by_mpidr
	mov	x30, x8
	ret
endfunc plat_my_core_pos


/* In:	x0 -  MPIDR_EL1
 * Out:	x0
 * Clobber list: x0,x1
 */
func s32g_core_pos_by_mpidr
	and	x0, x0, #S32G_MPIDR_CPU_CLUSTER_MASK
	and	x1, x0, #S32G_MPIDR_CPU_MASK
	lsr	x0, x0, #S32G_MPIDR_CLUSTER_SHIFT
	add	x0, x1, x0, lsl #1
	ret
endfunc s32g_core_pos_by_mpidr


/* Clobber list: x7 */
func plat_core_pos_by_mpidr
	mov	x7, x30
	/* TODO validate MPIDR */
	bl	s32g_core_pos_by_mpidr
	mov	x30, x7
	ret
endfunc plat_core_pos_by_mpidr
