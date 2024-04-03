#
# Copyright 2023-2024 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/nxp/s32/s32cc/s32g/s32g2/s32g2.mk

S32_BOARD_PATH		:= ${PLAT_SOC_PATH}/s32g2xxaevb3

PLAT_INCLUDES		+= -I${S32_BOARD_PATH}/include \

DTB_FILE_NAME		?= s32g2xxa-evb3.dtb
