#
# Copyright 2019-2021 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/nxp/s32g/s32g_common.mk

PLAT_BL_COMMON_SOURCES	+= drivers/nxp/s32g/clk/s32g274a_clk.c \
			   plat/nxp/s32g/s32g2/s32g2_sramc.S \
			   lib/cpus/aarch64/s32g2.S \
			   lib/cpus/aarch64/cortex_a53.S \

ERRATA_S32G2_050481	:= 1
ERRATA_A53_855873	:= 1
ERRATA_A53_836870	:= 1
ERRATA_A53_1530924	:= 1

# Device tree
DTB_FILE_NAME		?= fsl-s32g274a-rdb.dtb
FDT_SOURCES             := $(addprefix fdts/, $(patsubst %.dtb,%.dts,$(DTB_FILE_NAME)))
