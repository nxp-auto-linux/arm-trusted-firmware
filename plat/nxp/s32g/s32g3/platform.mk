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

include plat/nxp/s32g/s32g_common.mk

PLAT_INCLUDES		+= -Iplat/nxp/s32g/s32g3/include \

PLAT_BL_COMMON_SOURCES	+= plat/nxp/s32g/s32g3/s32g3_sramc.S \
			   plat/nxp/s32g/s32g3/s32g3_mc_me.c \
			   plat/nxp/s32g/s32g3/s32g3_mc_rgm.c \
			   drivers/nxp/s32g/clk/s32g398a_clk.c \
			   lib/cpus/aarch64/cortex_a53.S \

# Device tree
DTB_FILE_NAME		?= fsl-s32g398a-rdb.dtb
FDT_SOURCES             := $(addprefix fdts/, $(patsubst %.dtb,%.dts,$(DTB_FILE_NAME)))
