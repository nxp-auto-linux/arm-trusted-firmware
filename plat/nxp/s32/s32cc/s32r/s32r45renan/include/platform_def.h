/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

/* Remap memory layout.
 * Since the board only has 1G DDR/DDRM, everything needs to be moved to the
 * 0x80000000 - 0xB8000000 range (with ECC enabled).
 */

#define S32_PLATFORM_DDR0_END			0xb7ffffff
#define S32_PLATFORM_OSPM_SCMI_MEM		(0xa0000000U)

#include <s32r_platform_def.h>

#define S32GEN1_QSPI_100MHZ

#endif /* PLATFORM_DEF_H */
