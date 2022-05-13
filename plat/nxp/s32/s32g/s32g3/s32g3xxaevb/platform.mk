#
# Copyright 2022 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/nxp/s32/s32g/s32g3/s32g3.mk

S32_BOARD_PATH		:= ${PLAT_SOC_PATH}/s32g3xxaevb

PLAT_INCLUDES		+= -I${S32_BOARD_PATH}/include \

DTB_FILE_NAME		?= s32g3xxa-evb.dtb
