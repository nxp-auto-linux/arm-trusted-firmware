#
# Copyright 2024-2026 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

S32_PLATFORM_DDR0_END	?= 0xCFFFFFFF

S32_SCMI_SUBVENDOR_NAME := "\"S32R47\""

include plat/nxp/s32/s32cc/s32r/s32r_common.mk

PLAT_SOC_PATH	:= ${S32_SOC_FAMILY}/s32r47

PLAT_INCLUDES	+= -I${PLAT_SOC_PATH}/include \

# The structure of the SPIN_TABLE is the following
# byte 0: spin condition for core 0 (not used)
# byte 1: spin condition for core 1
# byte 2: spin condition for core 2
# byte 3: spin condition for core 3
# byte 4-7: reserved padding (alignment for 64bit access)
# byte 8-15: jump address for core 0 (not used)
# byte 16-23: jump address for core 1
# byte 24-31: jump address for core 2
# byte 32-39: jump address for core 3
#
# In bl2 all cores will end up in bl2_entry_point.
# Core 0 will cold boot, while cores 1-3 will be stuck
# in plat_secondary_cold_boot_setup.
# When core 0 enters BL31, it will free cores 1-3
# and get them stuck into another spin table but located
# in BL31. This is needed because BL2 is not resident in
# memory and can be reclaimed after BL31 is loaded.
#
$(eval S32R47_BL2_SPIN_TABLE_ADDR = 0x$(shell $(call hexbc, ${BL2_BASE}, -, 0x10000)))
$(eval $(call add_define,S32R47_BL2_SPIN_TABLE_ADDR))

BL2_SOURCES     +=  \
		    ${PLAT_SOC_PATH}/s32r47_bl2_el3_plat_arch_setup.c \
		    ${PLAT_SOC_PATH}/s32r47_lowlevel_bl2.S \
		    ${PLAT_SOC_PATH}/s32r47_early_plat_setup.c \

BL31_SOURCES     +=  \
		    ${PLAT_SOC_PATH}/s32r47_bl31.c \
		    ${PLAT_SOC_PATH}/s32r47_psci.c \
		    ${PLAT_SOC_PATH}/s32r47_lowlevel_bl31.S \

S32_LINFLEX_MODULE := 0

PLAT_BL_COMMON_SOURCES	+= ${S32_DRIVERS}/clk/s32r47_clk.c \
