#
# Copyright 2019-2020 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include lib/xlat_tables_v2/xlat_tables.mk

DDR_DRV = drivers/nxp/s32g/ddr

PLAT_INCLUDES		+= -Iplat/s32g/include \
			   -Iinclude/common/tbbr \
			   -Iinclude/plat/common \
			   -Iinclude/plat/arm/common \
			   -Iinclude/plat/arm/soc/common \
			   -Iinclude/lib \
			   -Iinclude/lib/libc \
			   -Iinclude/drivers \
			   -Iinclude/lib/psci

BL2_AT_EL3		:= 1


PLAT_BL_COMMON_SOURCES	+= plat/s32g/s32g_lowlevel_common.S \
			   plat/s32g/s32g_linflexuart.S \
			   plat/s32g/s32g_mc_me.c \
			   plat/s32g/s32g_ncore.c \
			   lib/cpus/aarch64/cortex_a53.S

BL2_SOURCES		+= plat/s32g/s32g_lowlevel_bl2.S \
			   plat/s32g/s32g274a_bl2_el3.c \
			   plat/s32g/s32g_clocks.c \
			   plat/s32g/s32g_pinctrl.c \
			   plat/s32g/s32g274a_storage.c \
			   plat/s32g/s32g274a_edma.c \
			   drivers/io/io_storage.c \
			   common/desc_image_load.c \
			   drivers/nxp/s32g/io/io_memmap.c \
			   ${DDR_DRV}/ddrss.c \
			   ${DDR_DRV}/ddrss_firmware.c \
			   ${DDR_DRV}/ddrss_regconf.c

BL31_SOURCES		+= plat/s32g/s32g275_bl31.c \
			   plat/s32g/s32g_psci.c \
			   plat/s32g/s32g274a_pm.c \
			   plat/common/plat_psci_common.c \
			   plat/common/plat_gicv3.c \
			   drivers/arm/gic/v3/gicv3_main.c \
			   drivers/arm/gic/v3/gicv3_helpers.c \
			   drivers/arm/gic/common/gic_common.c
BL31_SOURCES		+= plat/s32g/bl31_lowlevel.S \
			   plat/s32g/include/plat_macros.S
BL31_SOURCES		+= ${XLAT_TABLES_LIB_SRCS}


# Disable the PSCI platform compatibility layer
ENABLE_PLAT_COMPAT	:= 0

MULTI_CONSOLE_API	:= 1
LOAD_IMAGE_V2		:= 1
USE_COHERENT_MEM	:= 0

# Set RESET_TO_BL31 to boot from BL31
PROGRAMMABLE_RESET_ADDRESS	:= 1
RESET_TO_BL31			:= 0
# We need SMP boot in order to make specific initializations such as
# secure GIC registers, which U-Boot and then Linux won't be able to.
COLD_BOOT_SINGLE_CPU		:= 0
# Using Ncore as the cache coherency interconnect
HW_ASSISTED_COHERENCY		:= 1

### Platform-specific defines ###
# Which LinFlexD to use as a UART device
S32G_LINFLEX_MODULE	:= 0
$(eval $(call add_define_val,S32G_LINFLEX_MODULE,$(S32G_LINFLEX_MODULE)))


### Devel & Debug options ###
CFLAGS			+= -O0
# Enable dump of processor register state upon exceptions while running BL31
CRASH_REPORTING		:= 1
# As verbose as it can be
LOG_LEVEL		:= 50
