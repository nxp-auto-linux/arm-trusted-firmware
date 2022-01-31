#
# Copyright 2021-2022 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

S32_PLAT_SOC := s32r

DDR_DRV_SRCS            += ${DDR_DRV}/s32r/ddrc_cfg.c \
			   ${DDR_DRV}/s32r/dmem_cfg.c \
			   ${DDR_DRV}/s32r/dq_swap_cfg.c \
			   ${DDR_DRV}/s32r/phy_cfg.c \
			   ${DDR_DRV}/s32r/pie_cfg.c \

include plat/nxp/s32/s32_common.mk

PLAT_INCLUDES	+=	-Iplat/nxp/s32/s32r/include \
					-Iplat/nxp/s32/include \

PLAT_BL_COMMON_SOURCES += drivers/nxp/s32/clk/s32r45_clk.c \
		plat/nxp/s32/s32gen1_mc_me.c \
		plat/nxp/s32/s32gen1_mc_rgm.c \
		plat/nxp/s32/s32gen1_sramc.c \
		lib/cpus/aarch64/s32.S \
		lib/cpus/aarch64/cortex_a53.S \

BL2_SOURCES 	+=  \
	plat/nxp/s32/s32r/s32r_bl2_el3.c \

BL31_SOURCES += \
		   drivers/nxp/s32/clk/s32r_scmi_ids.c \
	       plat/nxp/s32/s32r/s32r_bl31.c \

ERRATA_S32_050481	:= 1
ERRATA_S32_050543	:= 1

# Which LinFlexD to use as a UART device
S32_LINFLEX_MODULE := 0
$(eval $(call add_define_val,S32_LINFLEX_MODULE,$(S32_LINFLEX_MODULE)))

DTB_FILE_NAME		?= fsl-s32r45-evb.dtb