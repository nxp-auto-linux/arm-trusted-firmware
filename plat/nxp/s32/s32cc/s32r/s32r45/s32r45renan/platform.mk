#
# Copyright 2023-2024, 2026 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

PLAT_BOARD = s32r45renan

S32_PLATFORM_DDR0_END	?= 0xb7ffffff

include plat/nxp/s32/s32cc/s32r/s32r45/s32r45.mk

S32_BOARD_PATH	:= ${PLAT_SOC_PATH}/s32r45renan

PLAT_INCLUDES	+= -I${S32_BOARD_PATH}/include \

DTB_FILE_NAME	= s32r45-renan.dtb
