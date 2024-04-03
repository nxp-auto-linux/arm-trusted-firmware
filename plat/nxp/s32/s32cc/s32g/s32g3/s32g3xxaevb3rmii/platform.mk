#
# Copyright 2022, 2024 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/nxp/s32/s32cc/s32g/s32g3/s32g3.mk

S32_BOARD_PATH		:= ${PLAT_SOC_PATH}/s32g3xxaevb3rmii

PLAT_INCLUDES		+= -I${S32_BOARD_PATH}/include \

DTB_FILE_NAME		?= s32g3xxa-evb3-rmii.dtb
