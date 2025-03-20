#
# Copyright 2020-2022, 2024-2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/nxp/s32/s32cc/s32_common.mk
include plat/nxp/s32/s32cc/s32_ddr.mk
include drivers/nxp/s32/s32cc/ddr/ddr_erratum/s32_derating_erratum.mk

S32_SOC_FAMILY	:= ${S32CC_PLAT}/s32g

S32_VR5510 ?= 0
$(eval $(call add_define_val,S32_VR5510,$(S32_VR5510)))

include ${S32_SOC_FAMILY}/bl31_sram/bl31_sram.mk
include ${S32CC_PLAT}/fconf/s32g/fconf.mk

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
			   ${S32CC_PLAT}/s32_ddr_utils.c \
			   ${S32CC_PLAT}/s32cc_lowlevel_bl2.S \

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

# Temporary buffer used by the io block layer is placed in
# SRAM before BL2 end (BL2_LIMIT).
S32_IO_BUFFER_SIZE	:= 0x013000
$(eval $(call add_define,S32_IO_BUFFER_SIZE))
