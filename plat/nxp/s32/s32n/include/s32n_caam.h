/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32N_CAAM_H
#define S32N_CAAM_H

#include <hse_srv_msc_utils.h>

/*
 * S32N7x has 2 CAAM instances (CAAM0 and CAAM1), matching HSE_MSC_ID_CAAM_0
 * and HSE_MSC_ID_CAAM_1. Compute the HSE MSC id of a CAAM instance from its
 * index (0 or 1), as derived from the device tree by the 'caam_config' FCONF
 * getter, instead of hard-coding HSE_MSC_ID_CAAM_0.
 */
static inline hseMscId_t s32n_caam_msc_id(unsigned int instance)
{
	return (hseMscId_t)(HSE_MSC_ID_CAAM_TYPE_MASK | instance);
}

#endif /* S32N_CAAM_H */
