/* SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause */
/*
 * Copyright 2021 NXP
 */
#include <asm_macros.S>
#include <cortex_a53.h>
#include <cpu_macros.S>

#if REPORT_ERRATA
/*
 * Errata printing function for S32G2 family.
 */
func s32g2_errata_report
	stp	x8, x30, [sp, #-16]!

	bl	cpu_get_rev_var
	mov	x8, x0

	/*
	 * Report all errata. The revision-variant information is passed to
	 * checking functions of each errata.
	 */
	report_errata ERRATA_S32G2_050481, s32g2, 050481

	bl cortex_a53_errata_report

	ldp	x8, x30, [sp], #16
	ret
endfunc s32g2_errata_report
#endif

func s32g2_cpu_reg_dump
	bl cortex_a53_cpu_reg_dump
endfunc s32g2_cpu_reg_dump

func check_errata_050481
	/* Applies to all revisions */
	mov x0, #ERRATA_APPLIES
	ret
endfunc check_errata_050481

declare_cpu_ops s32g2, CORTEX_A53_MIDR, \
	cortex_a53_reset_func, \
	cortex_a53_core_pwr_dwn, \
	cortex_a53_cluster_pwr_dwn
