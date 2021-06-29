/*
 * Copyright 2019-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <console_macros.S>
#include "platform_def.h"
#include "s32g_sramc.h"
#include <lib/utils_def.h>

.globl plat_is_my_cpu_primary
.globl plat_my_core_pos
.globl plat_core_pos_by_mpidr
.globl plat_panic_handler
.globl _s32g_sram_clr
.globl s32g_ncore_isol_cluster0
.globl reset_registers_for_lockstep

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
func s32g_ncore_isol_cluster0
	movz	x8, #S32G_NCORE_CAIU0_BASE_ADDR_H, lsl #16
	ldr	x9, [x8, #NCORE_CAIUTC_OFF]
	movz	x10, #1
	lsl	x10, x10, #NCORE_CAIUTC_ISOLEN_SHIFT
	orr	x9, x9, x10
	str	x9, [x8, #NCORE_CAIUTC_OFF]

	ret
endfunc s32g_ncore_isol_cluster0

/*
 * Translate a bus address in SRAM controller initialization address based on:
 * mem_addr[16:0] = {bus_addr[23:9], bus_addr[5:4]}
 *
 * addr := ((addr >> 9) << 2) | ((addr >> 4) & 0x3);
 *
 * x0: SRAM bus address
 * ret: x0: Initialization address
 *
 * Clobber list: x0,x4,x5
 */
func calc_sramc_addr
	lsr	x4, x0, #9
	lsl	x4, x4, #2
	lsr	x5, x0, #4
	and	x5, x5, #3
	orr	x0, x4, x5
	ret
endfunc calc_sramc_addr

/**
 * Check if two intervals are in overlap
 *
 * x0: start index of the first interval
 * x1: end index of the first interval
 * x2: start index of the second interval
 * x3: end index of the second interval
 *
 * x0: return 0 if the intervals do not overlap and 1 otherwise
 * Clobber list: x0,x1,x2,x3,x4,x5,x6
 */
func in_overlap
	/* x4 = max */
	cmp	x1, x3
	b.gt	1f
	mov	x4, x3
	b	2f
1:
	mov	x4, x1
2:
	/* x5 = min */
	cmp	x0, x2
	b.lt	3f
	mov	x5, x2
	b	4f
3:
	mov	x5, x0
4:
	/* x5 = max - min */
	sub	x5, x4, x5
	/* x4 = first interval size */
	sub	x4, x1, x0
	/* x6 = second interval size */
	sub	x6, x3, x2
	/* x4 = len1 + len2 */
	add	x4, x4, x6
	cmp	x4, x5
	bgt	overlap
	mov	x0, #0
	ret
overlap:
	mov	x0, #1
	ret
endfunc in_overlap

/**
 * Clear SRAM range using SRAM controller
 *
 * x0: SRAM controller address
 * x1: Initialization Start Address
 * x2: Initialization End Address
 *
 * Clobber list: x0,x1,x2,x3
 */
func clear_sramc_range
	/* Disable the module */
	mov	x3, #0
	str	w3, [x0, #SRAMC_PRAMCR_OFFSET]

	/* Address range */
	str	w1, [x0, #SRAMC_PRAMIAS_OFFSET]
	str	w2, [x0, #SRAMC_PRAMIAE_OFFSET]

	/* Request initialization */
	mov	x3, #SRAMC_PRAMCR_INITREQ
	str	w3, [x0, #SRAMC_PRAMCR_OFFSET]

wait_init:
	ldr	w3, [x0, #SRAMC_PRAMSR_OFFSET]
	and	w3, w3, #SRAMC_PRAMSR_IDONE
	cbz	w3, wait_init

	/* Clear status */
	mov	w3, #SRAMC_PRAMSR_IDONE
	str	w3, [x0, #SRAMC_PRAMSR_OFFSET]

	ret
endfunc clear_sramc_range

/* x0: start address of memory area to clear
 * x1: end address of memory area to clear
 *
 * Clobber list: x0,x1,x9,x10
 */
func a53_sram_clr
	mov	x10, #0x0
	cmp	x1, x0
	bgt	pos_step
	mov	x9, #-8
	b	clr_loop
pos_step:
	mov	x9, #8
clr_loop:
	cmp	x1, x0
	beq	finish
	str	x10, [x0]
	add	x0, x0, x9
	b	clr_loop
finish:
	ret
endfunc a53_sram_clr

func clear_unaligned_ends
	/* Save x30 */
	mov	x12, x30
	/*
	 * Obtain SRAM addresses
	 */
	mov	x9, #S32G_SRAM_BASE
	add	x11, x1, x9
	add	x9, x0, x9

	and	x1, x9, #SRAM_INV_BLOCK_MASK
	cmp	x1, x9
	beq	clear_end
	add	x1, x1, SRAM_BLOCK
	mov	x0, x9
	bl	a53_sram_clr

clear_end:
	and	x0, x11, #SRAM_INV_BLOCK_MASK
	cmp	x0, x11
	beq	clr_exit
	mov	x1, x11
	bl	a53_sram_clr
clr_exit:
	mov	x30, x12
	ret
endfunc clear_unaligned_ends

/* x0: start address of memory area to clear
 * x1: end address of memory area to clear
 * x0: return 0 on error or size of memory cleared on success
 *
 * Clobber list: x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x11,x12,x13,x14,x15
 */
func _s32g_sram_clr
	/* Save x30 */
	mov	x15, x30

	mov	x13, x0
	mov	x14, x1
	bl	clear_unaligned_ends
	mov	x0, x13
	mov	x1, x14

	/* Align to SRAM blocks */
	and	x9, x0, #SRAM_INV_BLOCK_MASK
	cmp	x0, x9
	beq	align_high_addr
	add	x0, x9, SRAM_BLOCK

align_high_addr:
	and	x1, x1, #SRAM_INV_BLOCK_MASK

	sub	x1, x1, #1
	/* Low address */
	bl	calc_sramc_addr
	mov	x7, x0
	mov	x0, x1

	/* High address */
	bl	calc_sramc_addr

	/**
	 * x7 - Low initialization address
	 * x8 - High initialization address
	 */
	mov	x8, x0
	mov	x9, #SRAMC1_MAX_ADDR
	cmp	x8, x9
	bgt	error

	mov	x0, #SRAMC0_MIN_ADDR
	mov	x1, #SRAMC0_MAX_ADDR
	mov	x2, x7
	mov	x3, x8
	bl	in_overlap
	cbnz	x0, init_sramc0

check_sramc1:
	mov	x0, #SRAMC1_MIN_ADDR
	mov	x1, #SRAMC1_MAX_ADDR
	mov	x2, x7
	mov	x3, x8
	bl	in_overlap
	cbnz	x0, init_sramc1
	b	sram_exit

init_sramc0:
	mov	x0, #SRAMC0_BASE_ADDD_L
	movk	x0, #SRAMC0_BASE_ADDD_H, lsl #16
	mov	x1, x7
	mov	x2, x8
	mov	x9, #SRAMC0_MAX_ADDR
	cmp	x2, x9
	b.gt	1f
	b	2f
1:
	mov	x2, #SRAMC0_MAX_ADDR
2:
	bl	clear_sramc_range
	b	check_sramc1

init_sramc1:
	mov	x0, #SRAMC1_BASE_ADDD_L
	movk	x0, #SRAMC1_BASE_ADDD_H, lsl #16
	cmp	x7, #SRAMC1_MIN_ADDR
	b.lt	3f
	sub	x1, x7, #SRAMC1_MIN_ADDR
	b	4f
3:
	mov	x1, #0
4:
	sub	x2, x8, #SRAMC1_MIN_ADDR
	bl	clear_sramc_range

	mov	x0, #0

sram_exit:
	/* Restore x30 */
	mov	x30, x15
	ret

error:
	/* Restore x30 */
	mov	x30, x15
	mov	x0, #-1
	ret
endfunc _s32g_sram_clr

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
