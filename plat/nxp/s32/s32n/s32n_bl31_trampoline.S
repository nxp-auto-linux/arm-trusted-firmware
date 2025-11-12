/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>

.globl jump_to_bl31
.globl jump_to_bl31_end

func jump_to_bl31
    ldr x0, 1f
    br x0

1:
    .quad   bl31_warm_entrypoint
jump_to_bl31_end:
endfunc jump_to_bl31
