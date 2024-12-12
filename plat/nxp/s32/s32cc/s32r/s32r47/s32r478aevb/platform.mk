#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/nxp/s32/s32cc/s32r/s32r47/s32r47.mk

S32_BOARD_PATH	:= ${PLAT_SOC_PATH}/s32r478aevb

PLAT_INCLUDES	+= -I${S32_BOARD_PATH}/include \

DTB_FILE_NAME	?= s32r45-evb.dtb
