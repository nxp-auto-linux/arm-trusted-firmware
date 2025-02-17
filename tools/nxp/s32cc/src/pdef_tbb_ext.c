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
#include <pdef_tbb_key.h>

static ext_t pdef_tbb_ext[] = {
	[TRUSTED_OS_NVCOUNTER_EXT - TRUSTED_OS_NVCOUNTER_EXT] = {
		.oid = TRUSTED_OS_NVCOUNTER_OID,
		.opt = "tos-nvctr",
		.sn = "TrustedOSNVCounter",
		.ln = "Trusted OS Non-Volatile counter",
		.asn1_type = V_ASN1_INTEGER,
		.type = EXT_TYPE_NVCOUNTER,
		.attr.nvctr_type = NVCTR_TYPE_TFW
	},
	[DDR_FW_CONTENT_CERT_PK_EXT - TRUSTED_OS_NVCOUNTER_EXT] = {
		.oid = DDR_FW_CONTENT_CERT_PK_OID,
		.sn = "DDR FirmwareContentCertPK",
		.ln = "DDR Firmware content certificate public key",
		.asn1_type = V_ASN1_OCTET_STRING,
		.type = EXT_TYPE_PKEY,
		.attr.key = DDR_FW_CONTENT_KEY
	},
	[DDR_FW_NVCOUNTER_EXT - TRUSTED_OS_NVCOUNTER_EXT] = {
		.oid = DDR_FW_NVCOUNTER_OID,
		.opt = "ddrfw-nvctr",
		.sn = "DDRNVCounter",
		.ln = "DDR FW Non-Volatile counter",
		.asn1_type = V_ASN1_INTEGER,
		.type = EXT_TYPE_NVCOUNTER,
		.attr.nvctr_type = NVCTR_TYPE_TFW
	},
	[DDR_FW_HASH_EXT - TRUSTED_OS_NVCOUNTER_EXT] = {
		.oid = DDR_FW_HASH_OID,
		.opt = "ddr-fw",
		.help_msg = "DDR Firmware image file",
		.sn = "DDRFirmwareHash",
		.ln = "DDR Firmware hash (SHA256)",
		.asn1_type = V_ASN1_OCTET_STRING,
		.type = EXT_TYPE_HASH,
	},
};

PLAT_REGISTER_EXTENSIONS(pdef_tbb_ext);
