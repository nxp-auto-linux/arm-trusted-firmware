/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>

#if USE_TBBR_DEFS
#include <tbbr_oid.h>
#else
#include <platform_oid.h>
#endif

#include "ext.h"
#include <pdef_tbb_ext.h>

static ext_t pdef_tbb_ext[] = {
	[TRUSTED_OS_NVCOUNTER_EXT - TRUSTED_OS_NVCOUNTER_EXT] = {
		.oid = TRUSTED_OS_NVCOUNTER_OID,
		.opt = "tos-nvctr",
		.sn = "TrustedOSNVCounter",
		.ln = "Trusted OS Non-Volatile counter",
		.asn1_type = V_ASN1_INTEGER,
		.type = EXT_TYPE_NVCOUNTER,
		.attr.nvctr_type = NVCTR_TYPE_TFW
	}
};

PLAT_REGISTER_EXTENSIONS(pdef_tbb_ext);
