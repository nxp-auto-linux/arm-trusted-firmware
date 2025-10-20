#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#
# S32N79RDB Board Specific Definitions

# To be used in targets from common makefile
DTB_FILE_NAME   ?= s32n79-rdb.dtb

include plat/nxp/s32/s32n/s32n_common.mk

# Board specific build parameters
PLAT_S32N79RDB		:=  ${PLAT_S32N}/s32n79rdb

PLAT_INCLUDES += \
	-I${PLAT_S32N79RDB}/include \

