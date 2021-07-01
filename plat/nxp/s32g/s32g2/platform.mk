#
# Copyright 2019-2021 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/nxp/s32g/s32g_common.mk

PLAT_BL_COMMON_SOURCES	+= drivers/nxp/s32g/clk/s32g274a_clk.c \

# Device tree
DTB_FILE_NAME		?= fsl-s32g274a-rdb.dtb
FDT_SOURCES             := $(addprefix fdts/, $(patsubst %.dtb,%.dts,$(DTB_FILE_NAME)))
