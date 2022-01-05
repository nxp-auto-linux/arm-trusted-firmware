#
# Copyright 2020-2022 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/nxp/s32/s32_common.mk

ifeq ($(S32G_EMU),1)
DDR_DRV_SRCS := \
	${DDR_DRV}/emu/ddrss_emu.c \
	${DDR_DRV}/emu/ddrss_firmware_emu.c \
	${DDR_DRV}/emu/ddrss_regconf_emu.c \

endif

ifeq ($(S32G_EMU),0)
include plat/nxp/s32/s32g/bl31_sram/bl31_sram.mk
include plat/nxp/s32/s32g/bl31_ssram/bl31_ssram.mk
endif

PLAT_INCLUDES		+= -Iplat/nxp/s32/s32g/include \
			   -Iplat/nxp/s32/s32g/bl31_sram/include \
			   -Iplat/nxp/s32/s32g/bl31_ssram/include \

PLAT_BL_COMMON_SOURCES	+= \
			   plat/nxp/s32/s32g/s32g_mc_me.c \
			   plat/nxp/s32/s32g/s32g_bl_common.c \
			   plat/nxp/s32/s32g/s32g_pinctrl.c \
			   plat/nxp/s32/s32g/s32g_clocks.c \
			   drivers/nxp/s32/clk/s32g_clk.c \
			   drivers/nxp/s32/rst/s32gen1_rst.c \
			   drivers/nxp/s32/ocotp.c \
			   lib/utils/crc8.c \
			   plat/nxp/s32/s32g/s32g_vr5510.c \
			   drivers/nxp/s32/pmic/vr5510.c \
			   ${BL31SRAM_SRC_DUMP} \

BL2_SOURCES		+= \
			   plat/nxp/s32/s32g/s32g_bl2_el3.c \
			   ${BL31SSRAM_SRC_DUMP} \
			   lib/optee/optee_utils.c \

BL31_SOURCES		+= plat/nxp/s32/s32g/s32g_bl31.c \
			   plat/nxp/s32/s32g/s32g_psci.c \
			   plat/nxp/s32/s32g/s32g_resume.c \
			   plat/nxp/s32/s32g/s32g_pm.c \
			   plat/nxp/s32/s32g/s32g_svc.c \
			   plat/nxp/s32/s32g/s32g_scmi_clk.c \
			   plat/nxp/s32/s32g/s32g_scmi_rst.c \
			   drivers/nxp/s32/s32g_wkpu.c \
			   drivers/nxp/s32/clk/clk.c \
			   drivers/nxp/s32/clk/fixed_clk.c \
			   drivers/nxp/s32/clk/s32g_scmi_ids.c \
			   drivers/nxp/s32/clk/s32gen1_scmi_clk.c \
			   drivers/nxp/s32/clk/s32gen1_scmi_ids.c \
			   drivers/scmi-msg/base.c \
			   drivers/scmi-msg/clock.c \
			   drivers/scmi-msg/entry.c \
			   drivers/scmi-msg/reset_domain.c \

BL31_SOURCES		+= plat/nxp/s32/s32g/bl31_lowlevel.S \

### Platform-specific defines ###
# Which LinFlexD to use as a UART device
ifeq ($(S32G_EMU),0)
S32_LINFLEX_MODULE	:= 0
else
S32_LINFLEX_MODULE	:= 1
endif
$(eval $(call add_define_val,S32_LINFLEX_MODULE,$(S32_LINFLEX_MODULE)))

# Whether we're going to run a hypervisor (EL2) or jump straight into the
# bootloader (EL1)
S32G_HAS_HV		?= 0
$(eval $(call add_define_val,S32G_HAS_HV,$(S32G_HAS_HV)))
