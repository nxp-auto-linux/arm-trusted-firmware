#
# Copyright 2021-2024 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

S32_PLAT_SOC := s32r

include plat/nxp/s32/s32cc/s32_common.mk

S32_SOC_FAMILY 	:= ${S32CC_PLAT}/s32r

PLAT_INCLUDES	+=	-I${S32_SOC_FAMILY}/include \
			-Iinclude/${S32_DRIVERS}/ddr/s32r \

PLAT_BL_COMMON_SOURCES += \
		${S32CC_PLAT}/s32cc_mc_me.c \
		${S32CC_PLAT}/s32cc_mc_rgm.c \
		${S32CC_PLAT}/s32cc_sramc.c \
		${S32_SOC_FAMILY}/s32r_pinctrl.c \
		lib/cpus/aarch64/cortex_a53.S \
		${S32_SOC_FAMILY}/s32r_plat_funcs.c \

BL2_SOURCES 	+=  \
	${S32_SOC_FAMILY}/s32r_bl2_el3.c \

BL31_SOURCES += \
		${S32_DRIVERS}/clk/s32r_scmi_ids.c \
		${S32_SOC_FAMILY}/s32r_bl31.c \

# Which LinFlexD to use as a UART device
S32_LINFLEX_MODULE ?= 0
$(eval $(call add_define_val,S32_LINFLEX_MODULE,$(S32_LINFLEX_MODULE)))
