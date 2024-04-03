#
# Copyright 2022, 2024 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/nxp/s32/s32cc/s32g/s32g2/s32g2.mk

S32_BOARD_PATH		:= ${PLAT_SOC_PATH}/s32g274ardb2

PLAT_INCLUDES          += -I${S32_BOARD_PATH}/include \

PLAT_BL_COMMON_SOURCES +=      ${PLAT_SOC_PATH}/s32g2_vr5510.c \

DTB_FILE_NAME		?= s32g274a-rdb2.dtb
