#
# Copyright 2021 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

DDR_DRV_SRCS            += ${DDR_DRV}/s32g3/ddrc_cfg.c \
			   ${DDR_DRV}/s32g3/dmem_cfg.c \
			   ${DDR_DRV}/s32g3/dq_swap_cfg.c \
			   ${DDR_DRV}/s32g3/phy_cfg.c \
			   ${DDR_DRV}/s32g3/pie_cfg.c \

include plat/nxp/s32/s32g/s32g_common.mk

PLAT_INCLUDES		+= -Iplat/nxp/s32/s32g/s32g3/include \

PLAT_BL_COMMON_SOURCES	+= plat/nxp/s32/s32g/s32g3/s32g3_mc_me.c \
			   plat/nxp/s32/s32g/s32g3/s32g3_mc_rgm.c \
			   plat/nxp/s32/s32g/s32g3/s32g3_sramc.c \
			   plat/nxp/s32/s32g/s32g3/s32g3_vr5510.c \
			   drivers/nxp/s32/clk/s32g3_clk.c \
			   lib/cpus/aarch64/cortex_a53.S \

# Device tree
DTB_FILE_NAME		?= fsl-s32g399a-rdb.dtb
