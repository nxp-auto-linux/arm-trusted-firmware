/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <drivers/arm/gicv3.h>
#include "platform_def.h"

.globl _rename_this_plat_secondary_cold_boot_setup


/* Clear GICR_WAKER[ProcessorSleep] bit.
 * Returns in x8 the base address of the current Redistributor.
 *
 * Clobber list: x7,x8,x9,x10,x11
 */
func gicr_waker_wake
	mov	x11, x30
	mov	x7, x0
	bl	plat_my_core_pos

	/* Find offset of our Redistributor */
	mov	x8, #PLAT_GICR_BASE
dec:
	cbz	x0, clear_ps
	mov	x9, #2
	add	x8, x8, x9, lsl #16
	mov	x9, #1
	sub	x0, x0, x9
	b	dec
clear_ps:
	/* Clear the PS bit */
	ldr	w9, [x8, #GICR_WAKER]
	movz	w10, #1
	mvn	w10, w10, lsl #WAKER_PS_SHIFT
	and	w9, w9, w10
	str	w9, [x8, #GICR_WAKER]
	dsb	st
	isb
	/* Wait for hw confirmation */
children_asleep:
	ldr	w9, [x8, #GICR_WAKER]
	tbnz	w9, #WAKER_CA_SHIFT, children_asleep

	mov	x0, x7
	mov	x30, x11
	ret
endfunc gicr_waker_wake


/* GICR and GICC initializations for the secondary core.
 *
 * This is a close equivalent of gicv3_cpuif_enable() that we need to make
 * ourselves here, because:
 *   a) GICR_WAKER must be cleared before accessing most of the ICC_* regs;
 *   b) the ICC_* registers are not memory-mapped, so the primary core cannot
 *   do these initializations on our behalf.
 *
 * Clobber list: x7,x8,x9,x10,x11,x13
 */
func s32g_gic_fixups_for_secondary
	mov	x13, x30
	bl	gicr_waker_wake

	/* Clear any pending interrupts before we go into wfi */
	mov	x7, #0x1
	lsl	x7, x7, #16	/* #GICR_SGIBASE_OFFSET */
	add	x7, x7, #0x280	/* partial offset of #GICR_ICPENDR0 */
	mov	x9, xzr
	mvn	x9, x9
	str	w9, [x8, x7] /* x8 was set by gicr_waker_wake */
	dsb	sy

	/* SRE */
	mrs	x8, ICC_SRE_EL3
	orr	x8, x8, #0xf
	msr	ICC_SRE_EL3, x8
	isb
	/* Switch to NS state to write non secure ICC_SRE_EL2 and ICC_SRE_EL1 */
	mov	x8, xzr
	msr	SCR_EL3, x8
	isb
	/* ICC_SRE_EL2 and ICC_SRE_EL1 are left to the lower levels */

	/* Revert to secure state and set up the rest of SCR_EL3 */
	mov	x8, xzr
	orr	x8, x8, #SCR_RW_BIT
	/* Leave IRQ, FIQ, EA bits cleared */
	msr	SCR_EL3, x8
	isb

	/* Set interrupt priority filter */
	movz	x8, #0xff
	msr	ICC_PMR_EL1, x8
	isb

	msr	ICC_CTLR_EL3, xzr
	msr	ICC_CTLR_EL1, xzr

	/* Enable Group 0 (must come after ICC_PMR_EL1) */
	movz	x8, #0x1
	msr	ICC_IGRPEN0_EL1, x8

	isb

	mov	x30, x13
	ret
endfunc s32g_gic_fixups_for_secondary


/*
 * Clobber list: x0,x7,x8,x9,x10,x11,x13
 * FIXME rename this, it is no longer the entrypoint
 */
func _rename_this_plat_secondary_cold_boot_setup
	bl	s32g_gic_fixups_for_secondary
	bl	plat_my_core_pos
	lsl	x0, x0, #2	/* array elements are of size 32-bit*/
	ldr	x7, =s32g_core_release_var
pen:
	dsb	sy
	/* index with current core id */
	ldr	w8, [x7, x0]
	cbnz	w8, wfi_done
	wfi
	b	pen
wfi_done:
	/* reset condition variable */
	mov	x8, #0
	str	w8, [x7, x0]
	/* point of no return */
	b	bl31_warm_entrypoint
endfunc _rename_this_plat_secondary_cold_boot_setup
