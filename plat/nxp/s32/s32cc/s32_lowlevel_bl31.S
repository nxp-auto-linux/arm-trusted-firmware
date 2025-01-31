/*
 * Copyright 2019-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <drivers/arm/gicv3.h>
#include "platform_def.h"
#include "s32cc_ncore.h"

.weak plat_secondary_cold_boot_setup

.globl s32_core_release_var


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
