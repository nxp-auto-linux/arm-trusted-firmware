#
# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

TRUSTED_BOARD_BOOT 	:= 1
GENERATE_COT 		:= 1
INCLUDE_TBBR_MK 	:= 0

# !!! MUST BE 0(ZERO) IN A PRODUCTION ENVIRONMENT !!!
DYN_DISABLE_AUTH	:= 1

S32CC_MONOTONIC_CNT_SUPPORT ?= 0
$(eval $(call add_define,PLAT_DEF_OID))

ifeq (${NXP_HSE_FWDIR},)
$(error NXP_HSE_FWDIR is not set)
endif

ifeq ($(wildcard ${NXP_HSE_FWDIR}/interface/*),)
$(error ${NXP_HSE_FWDIR}/interface directory does not exist)
endif

PLAT_INCLUDES += -I${NXP_HSE_FWDIR}/interface \
		   $(addprefix -I,$(wildcard ${NXP_HSE_FWDIR}/interface/*/)) \
		 -Iinclude/${S32_DRIVERS}/hse \

BL2_SOURCES += ${S32_DRIVERS}/hse/hse_mu.c \
	       ${S32_DRIVERS}/hse/hse_core.c \
	       ${S32_DRIVERS}/hse/hse_utils.c \
	       ${S32_DRIVERS}/hse/hse_mem.c \

include drivers/auth/mbedtls/mbedtls_x509.mk

TBBR_SOURCES	:= drivers/auth/auth_mod.c \
		   drivers/auth/crypto_mod.c \
		   drivers/auth/img_parser_mod.c \
		   ${S32CC_PLAT}/tbbr/s32_crypto_lib.c \
		   ${S32CC_PLAT}/tbbr/s32_tbbr_cot.c \
		   ${S32CC_PLAT}/tbbr/s32_trusted_boot.c \
		   ${S32_DRIVERS}/auth/plat_img_parser.c \

ifeq (${S32CC_MONOTONIC_CNT_SUPPORT},1)
ifneq (${MMC_RPMB_SUPPORT},1)
$(error RPMB SUPPORT MUST BE ENABLED TO USE HSE MONOTONIC COUNTER SUPPORT)
else
RPMB_KEY_HSE_KEYHANDLE ?= 0x010200
RPMB_MONOTONIC_CNT_ADDR ?= 0x0
endif
else
RPMB_KEY_HSE_KEYHANDLE ?= 0x0
RPMB_MONOTONIC_CNT_ADDR ?= 0x0
endif
$(eval $(call add_defines,S32CC_MONOTONIC_CNT_SUPPORT RPMB_KEY_HSE_KEYHANDLE RPMB_MONOTONIC_CNT_ADDR))

TFW_NVCTR_VAL	?= 0
NTFW_NVCTR_VAL	?= 0
DDRFW_NVCTR_VAL ?= 0

# Pass the non-volatile counters to the cert_create tool
$(eval $(call CERT_ADD_CMD_OPT,${TFW_NVCTR_VAL},--tfw-nvctr))
ifeq (${SPD}, opteed)
TOS_NVCTR_VAL  ?= 0
$(eval $(call CERT_ADD_CMD_OPT,${TOS_NVCTR_VAL},--tos-nvctr))
endif
$(eval $(call CERT_ADD_CMD_OPT,${NTFW_NVCTR_VAL},--ntfw-nvctr))
$(eval $(call CERT_ADD_CMD_OPT,${DDRFW_NVCTR_VAL},--ddrfw-nvctr))

# BL2
ifeq (${KEY_ALG},rsa)
BL2_KEY ?= ${BUILD_PLAT}/bl2_rsa2048_private.pem
DEFAULT_HSE_KEYHANDLE = 0x010700
else
BL2_KEY ?= ${BUILD_PLAT}/bl2_ecc_private.pem
KEY_SIZE ?= 256
DEFAULT_HSE_KEYHANDLE = 0x010400
endif
ifeq (${BL2_KEY},)
$(error BL2_KEY file is not set)
endif

# Check if the BL2_KEY file exists
ifeq ($(wildcard $(BL2_KEY)),)
# If it doesn't exist, create it
$(BL2_KEY): create_bl2_key
# If it does, check that it's a valid PEM format RSA Key
else
$(BL2_KEY): check_bl2_key
endif

create_bl2_key:
	@${ECHO} "Creating new BL2 key"
ifeq (${KEY_ALG},rsa)
	@${OPENSSL} genrsa 2048 > ${BL2_KEY} 2>/dev/null
else ifeq (${KEY_ALG},ecdsa)
ifeq (${KEY_SIZE},384)
	@${OPENSSL} ecparam -name secp384r1 -genkey > ${BL2_KEY} 2>/dev/null
else
	@${OPENSSL} ecparam -name secp256r1 -genkey > ${BL2_KEY} 2>/dev/null
endif
else
	$(error ${KEY_ALG} not supported)
endif

check_bl2_key:
	@${ECHO} "Checking BL2 key"
ifeq (${KEY_ALG},rsa)
	@${OPENSSL} rsa -in ${BL2_KEY} -check -noout 2>/dev/null || { exit 1; }
else
	@${OPENSSL} ec -in ${BL2_KEY} -check -noout 2>/dev/null || { exit 1; }
endif

certificates: $(BL2_KEY)

# Instruct the cert_create tool to save the keys in 'BL3x_KEY' files
# We will need them for the later import in the HSE NVM Key Catalog
SAVE_KEYS := 1

# For each BL3x we need to:
#	1. Specify the key handle where the public RSA key will be imported
#	   in the HSE NVM Key Catalog. The key handle is a 32-bit integer:
#	   the key catalog(byte2), group index in catalog (byte1) and key slot index (byte0).
#	   This Key handle will be stored into the FIP. For more information on how a
#	   key handle is computed, please consult HSE Interface's
#	   inc_common/hse_keymgmt_common_types.h.
#	2. Specify the file where the cert_create tool will save the RSA key
#	   in PEM format. If the file exists, the tool will check if it contains an
#	   RSA Key and use it.
#	3. Create entries in the FIP Image for the key & content certificates
#
# By design, the HSE NVM Key Catalog has an upper limit of asymmetric keys that can
# be configured for the Standard HSE Firmware variant. This limit has been reached
# for the current NVM Catalog format. Hence, we'll use the same key that's used
# for signing BL2 for development purposes.
# ! SEPARATE KEYS SHOULD BE USED FOR EACH BL IMAGE IN A PRODUCTION ENVIRONMENT !

# BL31
BL31_HSE_KEYHANDLE ?= ${DEFAULT_HSE_KEYHANDLE}
BL31_KEY ?= ${BL2_KEY}
$(eval $(call CERT_ADD_CMD_OPT,${BL31_KEY},--soc-fw-key))

$(eval $(call TOOL_ADD_PAYLOAD,${BUILD_PLAT}/soc_fw_content.crt,--soc-fw-cert))
$(eval $(call TOOL_ADD_PAYLOAD,${BUILD_PLAT}/soc_fw_key.crt,--soc-fw-key-cert))

# BL32
ifeq (${SPD}, opteed)
BL32_HSE_KEYHANDLE ?= ${DEFAULT_HSE_KEYHANDLE}
BL32_KEY ?= ${BL2_KEY}
$(eval $(call CERT_ADD_CMD_OPT,${BL32_KEY},--tos-fw-key))

$(eval $(call TOOL_ADD_PAYLOAD,${BUILD_PLAT}/tos_fw_content.crt,--ctos-fw-cert))
$(eval $(call TOOL_ADD_PAYLOAD,${BUILD_PLAT}/tos_fw_key.crt,--tos-fw-key-cert))
endif

# BL33
BL33_HSE_KEYHANDLE ?= ${DEFAULT_HSE_KEYHANDLE}
BL33_KEY ?= ${BL2_KEY}
$(eval $(call CERT_ADD_CMD_OPT,${BL33_KEY},--nt-fw-key))

$(eval $(call TOOL_ADD_PAYLOAD,${BUILD_PLAT}/nt_fw_content.crt,--nt-fw-cert))
$(eval $(call TOOL_ADD_PAYLOAD,${BUILD_PLAT}/nt_fw_key.crt,--nt-fw-key-cert))

ifeq (${CUSTOM_DDR_DRV},)
DDR_HSE_KEYHANDLE ?= ${DEFAULT_HSE_KEYHANDLE}
DDRFW_KEY ?= ${BL2_KEY}

$(eval $(call CERT_ADD_CMD_OPT,${DDRFW_KEY},--ddr-fw-key))

$(eval $(call TOOL_ADD_PAYLOAD,${BUILD_PLAT}/ddr_fw_content.crt,--ddr-fw-cert))
$(eval $(call TOOL_ADD_PAYLOAD,${BUILD_PLAT}/ddr_fw_key.crt,--ddr-fw-key-cert))
endif

# Check if any two of the BL keys are equal
ifeq ($(BL2_KEY),$(BL31_KEY))
HSE_SECBOOT_WARNING := 1
$(eval $(call add_define,HSE_SECBOOT_WARNING))
endif
ifeq ($(BL2_KEY),$(BL33_KEY))
HSE_SECBOOT_WARNING := 1
$(eval $(call add_define,HSE_SECBOOT_WARNING))
endif
ifeq ($(BL31_KEY),$(BL33_KEY))
HSE_SECBOOT_WARNING := 1
$(eval $(call add_define,HSE_SECBOOT_WARNING))
endif
ifeq ($(DDRFW_KEY),$(BL2_KEY))
HSE_SECBOOT_WARNING := 1
$(eval $(call add_define,HSE_SECBOOT_WARNING))
endif
ifeq ($(DDRFW_KEY),$(BL31_KEY))
HSE_SECBOOT_WARNING := 1
$(eval $(call add_define,HSE_SECBOOT_WARNING))
endif
ifeq ($(DDRFW_KEY),$(BL33_KEY))
HSE_SECBOOT_WARNING := 1
$(eval $(call add_define,HSE_SECBOOT_WARNING))
endif
ifeq (${SPD}, opteed)
ifeq ($(BL2_KEY),$(BL32_KEY))
HSE_SECBOOT_WARNING := 1
$(eval $(call add_define,HSE_SECBOOT_WARNING))
endif
ifeq ($(BL32_KEY),$(BL31_KEY))
HSE_SECBOOT_WARNING := 1
$(eval $(call add_define,HSE_SECBOOT_WARNING))
endif
ifeq ($(BL32_KEY),$(BL33_KEY))
HSE_SECBOOT_WARNING := 1
$(eval $(call add_define,HSE_SECBOOT_WARNING))
endif
ifeq ($(DDRFW_KEY),$(BL32_KEY))
HSE_SECBOOT_WARNING := 1
$(eval $(call add_define,HSE_SECBOOT_WARNING))
endif
endif

HSE_SUPPORT := 1
call_mkimage: sign_image

define hex_to_bin
echo $1 | xxd -r -p | xxd -e | ${AWK} -F[:.] '{print $$2}' | xxd -r -p > $2
endef

define update_cert
${FIPTOOL} update --align ${FIP_ALIGN} $1 $2 ${FIP_BIN}
endef

BL2_VERSION ?= 0x00000000
BL2_VERSION_SPACE ?= 0x8
BL2_VERSION_OFFSET ?= 0x4
BL2_VERSION_FILE = ${BUILD_PLAT}/bl2_version.bin
BL2_VER_SIZE_FILE = ${BUILD_PLAT}/bl2_version_size

${LOAD_ADDR_FILE}: ${BL2_VER_SIZE_FILE}
${BL2_W_DTB_SIZE_FILE}: add_bl2_version

${BL2_VER_SIZE_FILE}: FORCE
	$(eval BL2_VER_ALIGNED_HEX = 0x$(shell $(call hexalign_to_fip, ${BL2_VERSION_SPACE})))
	${ECHO} ${BL2_VER_ALIGNED_HEX} > $@

${BL2_VERSION_FILE}: ${BL2_VER_SIZE_FILE}
	${Q}$(call hex_to_bin,${BL2_VERSION},${BUILD_PLAT}/tmp.bin)
	${Q}${DD} if=/dev/zero of=$@ bs=$$(printf "%d" ${BL2_VER_ALIGNED_HEX}) count=1
	${Q}${DD} if=${BUILD_PLAT}/tmp.bin of=$@ seek=$$(printf "%d" ${BL2_VERSION_OFFSET}) oflag=seek_bytes conv=notrunc
	${Q}rm ${BUILD_PLAT}/tmp.bin

.PHONY: sign_image add_bl2_version

add_bl2_version: ${BL2_VERSION_FILE} ${BL2_W_DTB}
	${Q}cat ${BL2_VERSION_FILE} ${BL2_W_DTB} > ${BUILD_PLAT}/tmp.bin
	${Q}mv ${BUILD_PLAT}/tmp.bin ${BL2_W_DTB}

sign_image: fiptool fip ${BL2_W_DTB}
	@${OPENSSL} dgst -sha1 -sign ${BL2_KEY} -out ${BUILD_PLAT}/bl2-signature.bin ${BL2_W_DTB}

	${Q}$(call hex_to_bin,${BL31_HSE_KEYHANDLE},${BUILD_PLAT}/bl31_hse_keyhandle.bin)
	${Q}$(call update_cert,--soc-fw-key-cert,${BUILD_PLAT}/bl31_hse_keyhandle.bin)

ifeq (${SPD}, opteed)
	${Q}$(call hex_to_bin,${BL32_HSE_KEYHANDLE},${BUILD_PLAT}/bl32_hse_keyhandle.bin)
	${Q}$(call update_cert,--tos-fw-key-cert,${BUILD_PLAT}/bl32_hse_keyhandle.bin)
endif

	${Q}$(call hex_to_bin,${BL33_HSE_KEYHANDLE},${BUILD_PLAT}/bl33_hse_keyhandle.bin)
	${Q}$(call update_cert,--nt-fw-key-cert,${BUILD_PLAT}/bl33_hse_keyhandle.bin)

ifeq (${CUSTOM_DDR_DRV},)
	${Q}$(call hex_to_bin,${DDR_HSE_KEYHANDLE},${BUILD_PLAT}/ddr_hse_keyhandle.bin)
	${Q}$(call update_cert,--ddr-fw-key-cert,${BUILD_PLAT}/ddr_hse_keyhandle.bin)
endif

	${ECHO} "BL and DDR signatures have been added to the fip\n"

ifneq (${HSE_SECBOOT_WARNING},)
	${ECHO} "TWO OR MORE BL AUTHENTICATION KEYS ARE EQUAL."
	${ECHO} "PLEASE USE SEPARATE KEYS IN A PRODUCTION ENVIRONMENT!\n"
endif
