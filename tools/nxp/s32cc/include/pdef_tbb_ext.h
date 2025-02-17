/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PDEF_TBB_EXT_H
#define PDEF_TBB_EXT_H

#include <tbbr/tbb_ext.h>

/* Plat Defined TBBR extensions */
enum {
	TRUSTED_OS_NVCOUNTER_EXT = FWU_HASH_EXT + 1,
	DDR_FW_CONTENT_CERT_PK_EXT,
	DDR_FW_NVCOUNTER_EXT,
	DDR_FW_HASH_EXT,
};

#endif /* PDEF_TBB_EXT_H */
