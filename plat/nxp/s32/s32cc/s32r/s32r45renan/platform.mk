#
# Copyright 2023-2024 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

PLAT_BOARD = s32r45renan

include plat/nxp/s32/s32cc/s32r/s32r.mk

PLAT_INCLUDES		+= -Iplat/nxp/s32/s32cc/s32r/s32r45renan/include \

DTB_FILE_NAME		= s32r45-renan.dtb
