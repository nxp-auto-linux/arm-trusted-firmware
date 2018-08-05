/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include "platform_def.h"

.globl plat_is_my_cpu_primary
.globl plat_secondary_cold_boot_setup
.globl platform_mem_init
.globl plat_my_core_pos
.globl plat_my_core_pos

/*
 * All these helper functions have been copied copied from
 * imx8_helpers.S (plat/imx) or ls_helpers.S (plat/layerscape).
 */
func plat_is_my_cpu_primary
	mrs	x0, mpidr_el1
	and	x0, x0, #(MPIDR_CPU_MASK)
	cmp	x0, #PLAT_PRIMARY_CPU
	cset	x0, eq
	ret
endfunc plat_is_my_cpu_primary

func plat_secondary_cold_boot_setup
	b	.
endfunc plat_secondary_cold_boot_setup

func platform_mem_init
	ret
endfunc platform_mem_init

func plat_my_core_pos
	mrs	x0, mpidr_el1
	and	x1, x0, #MPIDR_CPU_MASK
	and 	x0, x0, #MPIDR_CLUSTER_MASK
	add	x0, x1, x0, LSR #6
	ret
endfunc plat_my_core_pos
