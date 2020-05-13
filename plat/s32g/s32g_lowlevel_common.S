/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <console_macros.S>
#include "platform_def.h"
#include "s32g274a_edma.h"

.globl plat_is_my_cpu_primary
.globl plat_my_core_pos
.globl plat_core_pos_by_mpidr
.globl plat_panic_handler
.globl dma_mem_clr
.globl sram_clr

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

/* x0: start address of memory area to clear
 * x1: size of memory area to clear
 * x0: return 0 on error or size of memory cleared on success
 * Clobber list: x0,x9,x12,x13
 */

func dma_mem_clr
	orr	x12, x0, x1
	and 	x12, x12, #0x3f
	cbz 	x12, transfer_size_64bytes
	and 	x12, x12, #0x7
	cbz 	x12, transfer_size_8bytes
	/* Assert that the addresses are aligned on at least a 64bit
	 * boundary, otherwise the transfer to an unitialized
	 * sram would fail anyway.
	 */
	bl	plat_panic_handler

transfer_size_8bytes:
	ldr	x9, =EDMA0_TCDn_ATTR(DMA_CHANNEL_1)
	mov	w12, #0x0303 /* 8bytes transfer size */
	strh	w12, [x9]

	ldr	x9, =EDMA0_TCDn_DOFF(DMA_CHANNEL_1)
	mov	w12, #0x0008 /* increment by 8bytes */
	strh	w12, [x9]
	b	transfer_size_determined

transfer_size_64bytes:
	ldr	x9, =EDMA0_TCDn_ATTR(DMA_CHANNEL_1)
	mov	w12, #0x0606 /* 64bytes transfer size */
	strh	w12, [x9]

	ldr	x9, =EDMA0_TCDn_DOFF(DMA_CHANNEL_1)
	mov	w12, #0x0040 /* increment by 64bytes */
	strh	w12, [x9]
transfer_size_determined:

	ldr	x9, =EDMA0_TCDn_DADDR(DMA_CHANNEL_1)
	str 	w0, [x9]

	ldr	x9, =EDMA0_TCDn_SOFF(DMA_CHANNEL_1)
	strh	wzr, [x9]

	ldr	x9, =EDMA0_TCDn_SADDR(DMA_CHANNEL_1)
	ldr	x12, =initvar
	str	w12, [x9]

	ldr	x9, =EDMA0_TCDn_NBYTES_MLOFFNO(DMA_CHANNEL_1)
	str 	w1, [x9]

	mov	w12, #0x0001
	ldr	x9, =EDMA0_TCDn_CITER_ELINKNO(DMA_CHANNEL_1)
	strh	w12, [x9]
	ldr	x9, =EDMA0_TCDn_BITER_ELINKNO(DMA_CHANNEL_1)
	strh	w12, [x9]

	ldr	x9, =EDMA0_TCDn_CSR(DMA_CHANNEL_1)
	mov	w12, #0x0001
	strb	w12, [x9]

check_status:
	/* Check error status */
	ldr	x9, =EDMA0_MP_ES
	ldr	w12, [x9]
	and	w12, w12, #0x80000000
	movz	w13, #0x8000, lsl #16
	sub	w12, w12, w13
	cbz	w12, transfer_error

	/* Check transfer done */
	ldr	x9, =EDMA0_CHn_CSR(DMA_CHANNEL_1)
	ldr 	w12, [x9]
	and 	w12, w12, #0x40000000
	movz	w13, #0x4000, lsl #16
	sub	w12, w12, w13
	cbnz	w12, check_status

	/* Clear EDMA0_CHn_CSR:DONE bit */
	ldr	x9, =EDMA0_CHn_CSR(DMA_CHANNEL_1)
	movz	w12, #0x4000, lsl #16
	str	w12, [x9]

	mov	x0, x1
	ret

transfer_error:
	ldr	x9, =EDMA0_CHn_ES(DMA_CHANNEL_1)
	movz	w10, #0x8000, lsl #16
	str	w10, [x9]

	mov	x0, #0x0
	ret
endfunc dma_mem_clr

/* x0: start address of memory area to clear
 * x1: size of memory area to clear
 * x0: return 0 on error or size of memory cleared on success
 * Clobber list: x0,x1,x10,x11,x15
 */

func sram_clr
	add	x1, x0, x1

	/* save registers that are clobbered by dma_mem_clr */
	mov	x15, x30
	mov	x10, x0
	mov	x11, x1

	/* If the start address is not aligned to 64bytes, the ideal
	 * transfer size for edma, perform a suboptimal edma transfer
	 * on the region until the first 64byte boundary.
	 */
	and	x1, x10, #0x3f
	cbz	x1, unaligned_start_done
	mov 	x1, x10, lsr #6
	mov 	x1, x1, lsl #6
	add 	x1, x1, #0x40
	mov	x10, x1
	sub	x1, x1, x0
	bl	dma_mem_clr
unaligned_start_done:

	/* Forcefully align the end address on a 64byte boundary
	 * and request an edma transfer.
	 */
	mov	x0, x10
	mov 	x1, x11
	mov 	x1, x1, lsr #6
	mov 	x1, x1, lsl #6
	sub 	x1, x1, x0
	bl	dma_mem_clr

	/* Perform another suboptimal edma transfer if there is an
	 * unaligned leftover region at the end after we forcefully
	 * aligned the address above.
	 */
	mov	x0, x11
	mov 	x0, x0, lsr #6
	mov 	x0, x0, lsl #6
	mov 	x1, x11
	sub 	x1, x1, x0
	cbz 	x1, unaligned_end_done
	bl	dma_mem_clr
unaligned_end_done:

	mov	x30, x15
	ret
endfunc sram_clr

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

/* Used by 'dma_mem_clr' and required to be aligned to
 * edma's maximum transfer size, which is 64bytes.
 */
.align 6
initvar:
	.quad 0x0
	.quad 0x0
	.quad 0x0
	.quad 0x0
	.quad 0x0
	.quad 0x0
	.quad 0x0
	.quad 0x0
