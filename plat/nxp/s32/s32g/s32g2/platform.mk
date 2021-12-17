#
# Copyright 2019-2021 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

DDR_DRV_SRCS            += ${DDR_DRV}/s32g2/ddrc_cfg.c \
			   ${DDR_DRV}/s32g2/dmem_cfg.c \
			   ${DDR_DRV}/s32g2/dq_swap_cfg.c \
			   ${DDR_DRV}/s32g2/phy_cfg.c \
			   ${DDR_DRV}/s32g2/pie_cfg.c \

include plat/nxp/s32/s32g/s32g_common.mk

PLAT_INCLUDES		+= -Iplat/nxp/s32/s32g/s32g2/include \

PLAT_BL_COMMON_SOURCES	+= drivers/nxp/s32g/clk/s32g274a_clk.c \
			   plat/nxp/s32/s32g/s32g2/s32g2_mc_me.c \
			   plat/nxp/s32/s32g/s32g2/s32g2_mc_rgm.c \
			   plat/nxp/s32/s32g/s32g2/s32g2_sramc.c \
			   plat/nxp/s32/s32g/s32g2/s32g2_vr5510.c \
			   lib/cpus/aarch64/s32g2.S \
			   lib/cpus/aarch64/cortex_a53.S \

ERRATA_S32G2_050481	:= 1
ERRATA_S32G2_050543     := 1

# Device tree
DTB_FILE_NAME		?= fsl-s32g274a-rdb.dtb