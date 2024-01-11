#
# Copyright 2020-2022, 2024 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/nxp/s32/s32cc/s32_common.mk

S32_SOC_FAMILY	:= ${S32CC_PLAT}/s32g

S32_VR5510 ?= 0
$(eval $(call add_define_val,S32_VR5510,$(S32_VR5510)))

ifeq ($(S32CC_EMU),1)
DDR_DRV_SRCS := \
	${DDR_DRV}/emu/ddrss_emu.c \
	${DDR_DRV}/emu/ddrss_firmware_emu.c \
	${DDR_DRV}/emu/ddrss_regconf_emu.c \

endif

include ${S32_SOC_FAMILY}/bl31_sram/bl31_sram.mk

PLAT_INCLUDES		+= -I${S32_SOC_FAMILY}/include \
			   -I${S32_SOC_FAMILY}/bl31_sram/include \
			   -Iinclude/${S32_DRIVERS}/ddr/s32g \

PLAT_BL_COMMON_SOURCES	+= \
			   ${S32_SOC_FAMILY}/s32g_mc_me.c \
			   ${S32_SOC_FAMILY}/s32g_bl_common.c \
			   ${S32_SOC_FAMILY}/s32g_pinctrl.c \
			   ${S32_SOC_FAMILY}/s32g_clocks.c \
			   ${S32_DRIVERS}/clk/s32g_clk.c \
			   ${S32_DRIVERS}/ocotp.c \
			   ${S32_DRIVERS}/rtc/s32g_rtc.c \
			   lib/utils/crc8.c \
			   ${S32_SOC_FAMILY}/s32g_plat_funcs.c \
			   ${BL31SRAM_SRC_DUMP} \

BL2_SOURCES		+= \
			   ${S32_SOC_FAMILY}/s32g_bl2_el3.c \

BL31_SOURCES		+= ${S32_SOC_FAMILY}/s32g_bl31.c \
			   ${S32_SOC_FAMILY}/s32g_resume.c \
			   ${S32_SOC_FAMILY}/s32g_pm.c \
			   ${S32_DRIVERS}/s32g_wkpu.c \
			   ${S32_DRIVERS}/clk/s32g_scmi_ids.c \

ifeq ($(S32_VR5510),1)
PLAT_BL_COMMON_SOURCES	+= \
			   ${S32_SOC_FAMILY}/s32g_vr5510.c \
			   ${S32_DRIVERS}/pmic/vr5510.c \

endif

### Platform-specific defines ###
# Which LinFlexD to use as a UART device
ifeq ($(S32CC_EMU),0)
S32_LINFLEX_MODULE	?= 0
else
S32_LINFLEX_MODULE	?= 1
endif
$(eval $(call add_define_val,S32_LINFLEX_MODULE,$(S32_LINFLEX_MODULE)))

