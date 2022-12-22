/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DDR_PLAT_H_
#define DDR_PLAT_H_

#include <stdbool.h>
#include <plat/nxp/s32g/ssram_mailbox.h>

#define STORE_CSR_ENABLE
#define RETENTION_ADDR		BL31SSRAM_CSR_BASE

#endif /* DDR_PLAT_H_ */
