/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef S32_DERATING_ERRATUM_H
#define S32_DERATING_ERRATUM_H

#if (ERRATA_S32_050543 == 1)
void s32_set_ddr_erratum_polling_flag(uint8_t polling_value);
#else
static inline void s32_set_ddr_erratum_polling_flag(uint8_t polling_value)
{
}
#endif
#endif /* S32_DERATING_ERRATUM_H */
