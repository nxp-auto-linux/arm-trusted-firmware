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
.globl plat_secondary_cold_boot_setup
.globl s32g_smp_fixup

.globl s32g_core_release_var


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


func bl2_sram_init
	ret
endfunc bl2_sram_init


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


/* Set SMPEN bit on u-boot's behalf */
func s32g_smp_fixup
	mrs x14, S3_1_c15_c2_1
	orr x14, x14, #(1 << 6)
	msr S3_1_c15_c2_1, x14
	isb

	ret
endfunc s32g_smp_fixup


func plat_secondary_cold_boot_setup
	/* FIXME I WAS HERE */
	ret
endfunc plat_secondary_cold_boot_setup
