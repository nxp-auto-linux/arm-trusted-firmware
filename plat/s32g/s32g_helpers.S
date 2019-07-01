/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <console_macros.S>
#include <drivers/arm/gicv3.h>
#include "platform_def.h"

.globl plat_is_my_cpu_primary
.globl plat_secondary_cold_boot_setup
.globl platform_mem_init
.globl plat_my_core_pos
.globl plat_core_pos_by_mpidr

.globl console_s32g_register
.globl console_s32g_putc
.globl console_s32g_flush

.globl s32g_smp_fixup

.globl s32g_core_release_var

/* Set SMPEN bit on u-boot's behalf */
func s32g_smp_fixup
	mrs x14, S3_1_c15_c2_1
	orr x14, x14, #(1 << 6)
	msr S3_1_c15_c2_1, x14
	isb

	ret
endfunc s32g_smp_fixup

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
 */
func plat_secondary_cold_boot_setup
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
endfunc plat_secondary_cold_boot_setup

func platform_mem_init
	ret
endfunc platform_mem_init

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

/* int console_s32g_register(uintptr_t base,
 *			     uint32_t clk,
 *			     uint32_t baud,
 *			     struct console_s32g *console)
 * Hard-coded configuration: 8 data bits, no parity, 1 stop bit, no start bits
 *
 * In: x0 - UART register base address
 *     w1 - UART clock in Hz
 *     w2 - baud rate
 *     x3 - pointer to empty struct console_s32g
 * Out: x0 = 1 on success, 0 on error
 * Clobber list: x0,x1,x2,x3,x6,x7,x16,x17
 */
func console_s32g_register
	mov	x7, x30		/* back up return address */

	/* Set master mode and init mode */
	movz	w16, #0x10	/* LINCR1_MME */
	movz	w17, #0x1	/* LINCR1_INIT */
	orr	w16, w16, w17
	str	w16, [x0, #S32G_LINFLEX_LINCR1]

	/* wait for init mode entry */
	movz	w16, #0x1000	/* LINSR_LINS_INITMODE */
	movz	w17, #0xF000	/* LINSR_LINS_MASK */
wait_init_mode:
	ldr	w6, [x0, #S32G_LINFLEX_LINSR]
	and	w6, w6, w17
	eor	w6, w6, w16
	cbnz	w6, wait_init_mode

	/* Set UART bit */
	movz	w16, #0x1	/* UARTCR_UART */
	str	w16, [x0, #S32G_LINFLEX_UARTCR]

	/* hard-coded linflexd_serial_setbrg() result,
	 * working with UARTCR[ROSE]=0
	 * FIXME: real implementation needed */
	movz	w16, #72		/* ibr */
	str	w16, [x0, #S32G_LINFLEX_LINIBRR]
	movz	w16, #5		/* fbr */
	str	w16, [x0, #S32G_LINFLEX_LINFBRR]

#ifdef S32G_VIRTUAL_PLATFORM
	/* Set preset timeout register value. Otherwise, print is very slow. */
	movz	w16, #0xf
	str	w16, [x0, #S32G_LINFLEX_UARTPTO]
#endif /* S32G_VIRTUAL_PLATFORM */

	/* 8-bit data, no parity, Tx/Rx enabled, UART mode */
	movz	w16, #0x40	/* UARTCR_PC1 */
	movz	w17, #0x20	/* UARTCR_RXEN */
	orr	w16, w16, w17
	movz	w17, #0x10	/* UARTCR_TXEN */
	orr	w16, w16, w17
	movz	w17, #0x8	/* UARTCR_PC0 */
	orr	w16, w16, w17
	movz	w17, #0x2	/* UARTCR_WL0 */
	orr	w16, w16, w17
	movz    w17, #0x1       /* UARTCR_UART */
	orr     w16, w16, w17
	movz	w17, #0x200	/* UARTCR_RFBM */
	orr	w16, w16, w17
	movz	w17, #0x100	/* UARTCR_TFBM */
	orr	w16, w16, w17
#ifdef S32G_VIRTUAL_PLATFORM
	/* Note: Setting the ROSE bit without also setting OSR is illegal; this
	 * only works because of a functional simulator quirk. We're keeping it,
	 * though, for easier debugging and sped-up output (on VDK only!).
	 * Also, these settings only speed up the output; the input would be
	 * totally crippled, unless UARTCR[OSR] is also set (see s32g-uboot for
	 * the complete hack).
	 *
	 * To be revised on actual Si. */
	movz	w17, #0x80, lsl #16	/* UARTCRT_ROSE */
	orr	w16, w16, w17
#endif /* S32G_VIRTUAL_PLATFORM */
	str	w16, [x0, #S32G_LINFLEX_UARTCR]

	ldr	w16, [x0, #S32G_LINFLEX_LINCR1]
	movz	w17, #0x1	/* LINCR1_INIT */
	orn	w16, w17, w16
	movz	w17, 0xFFFF
	eor	w16, w16, w17
	str	w16, [x0, #S32G_LINFLEX_LINCR1]

	/* prepare to finish console registration */
	mov	x0, x3
	mov	x30, x7
	finish_console_register s32g

	movz	w0, 1
	ret
endfunc console_s32g_register

/* In:  w0 - character to be printed
 *      x1 - pointer to the console_s32g structure (FIXME: currently ignored)
 * Out: w0 - printed character on success, < 0 on error
 * Clobber list: x0,x1,x2,x5,x6
 */
func console_s32g_putc
	/* FIXME: Do not hardcode the UART base addr; instead, pass it via the
	          console struct */
	/* S32G_UART_BASE */
	movz	x6, #0x401C, lsl #16
	movk	x6, #0x8000

	/* if c == '\n', also put a '\r' beforehand */
	movz	w2, #0
	cmp	w0, #0xA
	b.ne	putc_this

	mov	w2, w0
	movz	w0, #0xD
putc_this:
tx_fifo_full:
	ldr	w5, [x6, #S32G_LINFLEX_UARTSR]
	tbnz	w5, #1, tx_fifo_full	/* UARTSR_DTFTFF */
	strb	w0, [x6, #S32G_LINFLEX_BDRL]

	cbz	w2, done
	mov	w0, w2
	movz	w2, #0
	b	putc_this
done:
	ret
endfunc console_s32g_putc

func console_s32g_flush
	ret
endfunc console_s32g_flush
