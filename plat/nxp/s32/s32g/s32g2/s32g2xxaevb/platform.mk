#
# Copyright 2022 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/nxp/s32/s32g/s32g2/s32g2.mk

PLAT_INCLUDES		+= -Iplat/nxp/s32/s32g/s32g2/s32g2xxaevb/include \

PLAT_BL_COMMON_SOURCES	+=	plat/nxp/s32/s32g/s32g2/s32g2xxaevb/s32g2xxaevb_vr5510.c \

DTB_FILE_NAME		?= fsl-s32g2xxa-evb.dtb