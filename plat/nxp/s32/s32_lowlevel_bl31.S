/*
 * Copyright 2019-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <drivers/arm/gicv3.h>
#include "platform_def.h"
#include "s32_ncore.h"

.globl s32_smp_fixup
.globl plat_secondary_cold_boot_setup

.globl s32_core_release_var


/* Set SMPEN bit on u-boot's behalf */
/* TODO check whether this function is still necessary in BL31; in cortex_a53.S
 * there's a cortex_a53_rest_func doing the same. */
func s32_smp_fixup
	mrs x14, S3_1_c15_c2_1
	orr x14, x14, #(1 << 6)
	msr S3_1_c15_c2_1, x14
	isb

	ret
endfunc s32_smp_fixup


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

	/* Switch to NS state to write non secure ICC_SRE_EL2 */
	mrs	x8, SCR_EL3
	orr	x8, x8, #SCR_NS_BIT
	msr	SCR_EL3, x8
	isb

	mrs	x8, ICC_SRE_EL2
	orr	x8, x8, #0xf
	msr	ICC_SRE_EL2, x8
	isb

	/* Revert to secure state and set up more of SCR_EL3 */
	mrs	x8, SCR_EL3
	/* ...clear NS and possibly HCE */
	mov	x9, #SCR_NS_BIT
#if (S32_HAS_HV == 0)
	orr	x9, x9, #SCR_HCE_BIT
#endif
	mvn	x9, x9
	and	x8, x8, x9
	msr	SCR_EL3, x8
	isb

	/* Write the secure ICC_SRE_EL1 register */
	movz	x8, #0x1
	msr	ICC_SRE_EL1, x8
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

	/* Enable Group 1 Secure interrupts */
	mrs x8, ICC_IGRPEN1_EL3
	orr x8, x8, #0xf
	msr ICC_IGRPEN1_EL3, x8

	isb

	mov	x30, x13
	ret
endfunc s32g_gic_fixups_for_secondary


/* Wait until CSADSER[DVMSNPEN1<n>] becomes set, meaning that we're safe
 * to run with caches enabled on this cluster.
 *
 * In: x0 - core pos (0..7)
 * Clobber list: x7, x8, x9, x13
 */
func wait_ncore_caiu_online
	mov	x13, x30

	/* Detect cluster id */
	mov	x7, #PLATFORM_CORE_COUNT
	cmp	x0, x7, asr #1

	blt	caiu0
	mov	x7, #0x2
	b	csadse0_addr
caiu0:
	mov	x7, #0x1
csadse0_addr:
	mov	x8, #NCORE_BASE_ADDR
	mov	x9, #CSR_OFF
	add	x8, x8, x9
caiu_loop:
	ldr	x9, [x8, #CSADSER_OFF]	/* CSADSE0 */
	and	x9, x9, x7
	cbz	x9, caiu_loop

	mov	x30, x13
	ret
endfunc wait_ncore_caiu_online


func plat_secondary_cold_boot_setup
	bl	reset_registers_for_lockstep
	/* Reset all callee-saved registers for secondary boot as they are not
	 * initialized up to this point.
	 */
	mov x19, #0
	mov x20, #0
	mov x21, #0
	mov x22, #0
	mov x23, #0

	bl	s32g_gic_fixups_for_secondary
	bl	plat_my_core_pos
	lsl	x0, x0, #2	/* array elements are of size 32-bit*/
	ldr	x7, =s32_core_release_var
wfi_pen:
	dsb	sy
	/* Invalidate cache before reading s32_core_release_var[my_core_pos];
	 * Save x0, x1 first.
	 */
	mov	x9, x0
	mov	x10, x1
	add	x0, x7, x0
	mov	x1, #4
	bl	inv_dcache_range
	mov	x0, x9
	mov	x1, x10
	/* index s32_core_release_var with current core id */
	ldr	w8, [x7, x0]
	cbnz	w8, wfi_done

	/* Make sure interrupts are taken to EL3 before going into wfi */
	mrs	x8, SCR_EL3
	orr	x9, x8, #SCR_FIQ_BIT
	orr	x9, x9, #SCR_IRQ_BIT
	msr	SCR_EL3, x9
	/* Wait for the primary core to finish GIC setup on my behalf */
	wfi
	/* Restore SCR_EL3 */
	msr	SCR_EL3, x8

	b	wfi_pen
wfi_done:
	/* Wait for the master core to perform additional initializations
	 * such as ncore_init for my cluster
	 */
	bl	plat_my_core_pos	/* x0: my core index */
	bl	wait_ncore_caiu_online
	/* point of no return */
	ldr	x7, =s32_warmboot_entry
	ldr	x7, [x7]
	br	x7
endfunc plat_secondary_cold_boot_setup

