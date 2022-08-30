#
# Copyright 2021-2022 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

S32_PLAT_SOC := s32r

include plat/nxp/s32/s32_common.mk

PLAT_SOC_PATH	:= ${S32_PLAT}/${S32_PLAT_SOC}

PLAT_INCLUDES	+=	-I${PLAT_SOC_PATH}/include \
			-I${S32_PLAT}/include \
			-Iinclude/${S32_DRIVERS}/ddr/s32r \

PLAT_BL_COMMON_SOURCES += ${S32_DRIVERS}/clk/s32r45_clk.c \
		${S32_PLAT}/s32gen1_mc_me.c \
		${S32_PLAT}/s32gen1_mc_rgm.c \
		${S32_PLAT}/s32gen1_sramc.c \
		lib/cpus/aarch64/s32.S \
		lib/cpus/aarch64/cortex_a53.S \
		${PLAT_SOC_PATH}/s32r_plat_funcs.c \

BL2_SOURCES 	+=  \
	${PLAT_SOC_PATH}/s32r_bl2_el3.c \

BL31_SOURCES += \
		   ${S32_DRIVERS}/clk/s32r_scmi_ids.c \
	       ${PLAT_SOC_PATH}/s32r_bl31.c \

ERRATA_S32_050481	:= 1
ERRATA_S32_050543	:= 1

# Which LinFlexD to use as a UART device
S32_LINFLEX_MODULE := 0
$(eval $(call add_define_val,S32_LINFLEX_MODULE,$(S32_LINFLEX_MODULE)))

DTB_FILE_NAME		?= s32r45-evb.dtb
