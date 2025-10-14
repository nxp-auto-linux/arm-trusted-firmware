#
# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#
# S32N79SIM Board Specific Definitions

# To be used in targets from common makefile
DTB_FILE_NAME   ?= s32n79-simulator.dtb

include plat/nxp/s32/s32n/s32n_common.mk

# Board specific build parameters
PLAT_S32N79SIM		:=  ${PLAT_S32N}/s32n79sim

include ${PLAT_S32N79SIM}/fconf/fconf.mk

PLAT_INCLUDES += \
	-I${PLAT_S32N79SIM}/include \

# Use Linux Kernel Image as BL33, instead of U-boot
S32N_LINUX_KERNEL_AS_BL33	?=	0
$(eval $(call add_define,S32N_LINUX_KERNEL_AS_BL33))

# Default S32N APS topology
PLATFORM_CLUSTER_COUNT	?= 4
$(eval $(call add_define,PLATFORM_CLUSTER_COUNT))
S32N_PLAT_PRIMARY_CPU	?= 0
$(eval $(call add_define,S32N_PLAT_PRIMARY_CPU))

# DDR definitions
DDR_START ?= 0x80000000
$(eval $(call add_define,DDR_START))
DDR_END ?= 0xFFFFFFFF
$(eval $(call add_define,DDR_END))
# Total memory reserved for the BL stages
PLAT_BL_MEM_SIZE ?= 0x100000
$(eval $(call add_define,PLAT_BL_MEM_SIZE))

ifeq ($(S32N_LINUX_KERNEL_AS_BL33), 1)
# Linux Kernel DDR offset
BL33_ENTRYPOINT_DDR_START_OFFSET ?= 0x1000000
# Linux Kernel load address (default 0x81000000)
BL33_ENTRYPOINT ?= 0x$(shell $(call hexbc, $(DDR_START), +, $(BL33_ENTRYPOINT_DDR_START_OFFSET)))
else
# U-Boot DDR offset
BL33_ENTRYPOINT_DDR_END_OFFSET ?= 0x85FFFF
# U-Boot load address (default 0xff7A0000)
BL33_ENTRYPOINT ?= 0x$(shell $(call hexbc, $(DDR_END), -, $(BL33_ENTRYPOINT_DDR_END_OFFSET)))
endif

$(eval $(call add_define,BL33_ENTRYPOINT))

ifeq ($(S32N_LINUX_KERNEL_AS_BL33), 1)
# UFS_DESC_SIZE is increased to allow more PRDT entries for larger OS images.
# The previous value (0x400) was insufficient for large transfers and could lead
# to partial or failed transfers. The new value (0x800) supports images up to
# approximately 28 MB. If a Linux Kernel image larger than 28 MB is used as BL33
# in the future, this value will need to be increased again to avoid descriptor
# space limitations.
UFS_DESC_SIZE = 0x800
else
UFS_DESC_SIZE = 0x400
endif

$(eval $(call add_define,UFS_DESC_SIZE))

# Boot source selection.
S32N_UFS_BOOT_SOURCE = 0
S32N_MMC_BOOT_SOURCE = 0

# The default boot storage for S32N platforms is the UFS.
# Possible boot source options: ufs, mmc.
S32N_BOOT_SRC ?= ufs

ifeq ($(S32N_BOOT_SRC), ufs)
	PLAT_PARTITION_BLOCK_SIZE := 4096
	S32N_UFS_BOOT_SOURCE = 1
	S32N_CUSTOM_BOOTARGS ?= "\"root=/dev/sdb2 rootwait rw earlycon arm64.nopauth clk_ignore_unused\""
$(eval $(call add_define,PLAT_PARTITION_BLOCK_SIZE))
else ifeq ($(S32N_BOOT_SRC), mmc)
	S32N_MMC_BOOT_SOURCE = 1
	S32N_CUSTOM_BOOTARGS ?= "\"root=/dev/mmcblk0p4 rootwait rw earlycon arm64.nopauth clk_ignore_unused\""
else
$(error Unknown boot source $(S32N_BOOT_SRC). Please choose between: ufs, mmc)
endif

$(eval $(call add_define,S32N_UFS_BOOT_SOURCE))
$(eval $(call add_define,S32N_MMC_BOOT_SOURCE))
$(eval $(call add_define,S32N_CUSTOM_BOOTARGS))

PLAT_PARTITION_MAX_ENTRIES := 3
$(eval $(call add_define,PLAT_PARTITION_MAX_ENTRIES))

S32N_FIP_SIZE ?= 0x100000
$(eval $(call add_define,S32N_FIP_SIZE))

# Choose which PL011 module to use
S32N_PL011_MODULE ?= 0
$(eval $(call add_define,S32N_PL011_MODULE))

# Default value for APS peripheral base address.
# It represents the configurable SSID part: bits [31:20] from the Generic
# Peripheral Mapping scheme. This address should be configured from
# FSS FW TLV entries. For now, we can consider S=0x004.
APS_PERIPH_BASE ?= 0x400000
$(eval $(call add_define,APS_PERIPH_BASE))

# Choose the SCMI APS-FSS mailbox number and size
S32N_SCMI_MBX ?= 0
$(eval $(call add_define,S32N_SCMI_MBX))

PLAT_BL_COMMON_SOURCES += \
	${PLAT_S32N79SIM}/plat_helpers.S \
	${PLAT_S32N79SIM}/s32n_bl_common.c \
	${PLAT_S32N79SIM}/s32n_fss_scmi.c \
	${PLAT_S32N79SIM}/s32n_fss_utils.c \

BL2_SOURCES += \
	${PLAT_S32N79SIM}/s32n_bl_common.c \
	${PLAT_S32N79SIM}/s32n_bl2_el3_setup.c \
	${PLAT_S32N79SIM}/s32n_io_storage.c \

BL31_SOURCES += \
	${PLAT_S32N79SIM}/s32n_bl31_setup.c \
	${PLAT_S32N79SIM}/s32n_plat_funcs.c \
	${PLAT_S32N79SIM}/s32n_psci.c \
	${PLAT_S32N79SIM}/s32n_svc.c \
	${PLAT_S32N79SIM}/s32n_topology.c \

FCONF_GEN_SRC	:= ${BUILD_PLAT}/fconf_extractor_settings.c

FCONF_EXTRACTOR_BIN := ${BUILD_PLAT}/tools/fconf_extractor/fconf_extractor

GDB_VAR_DUMP = gic_config scmi_ospm_ch_config scmi_fss_ch_config scmi_sh_mem_config sdhc_config ufs_config
GDB_VAR_INCLUDES = s32n_fconf_sdhc_getter.h s32n_fconf_ufs_getter.h s32n_fconf_scmi_getter.h s32n_fconf_gic_getter.h

FCONF_EXTRACTOR_SRC_DIR = "tools/nxp/fconf_extractor"

FCONF_SRCS_PLAT_DIR := ${PLAT_S32N79SIM}/fconf

FCONF_EXTR_PLAT_SOURCES := \
	${FCONF_SRCS_PLAT_DIR}/plat_fconf_extractor.c \
	${FCONF_SRCS_PLAT_DIR}/s32n_fconf_gic_getter.c \
	${FCONF_SRCS_PLAT_DIR}/s32n_fconf_scmi_getter.c \
	${FCONF_SRCS_PLAT_DIR}/s32n_fconf_sdhc_getter.c \
	${FCONF_SRCS_PLAT_DIR}/s32n_fconf_ufs_getter.c \

FCONF_EXTR_PLAT_INCLUDE_DIRS := \
	${PLAT_S32N79SIM}/include/fconf \
	${PLAT_S32N79SIM}/include \

${FCONF_GEN_SRC}: ${PLAT_DTB_FILE}
	${Q}${MAKE} --no-print-directory -C "${FCONF_EXTRACTOR_SRC_DIR}" \
		BUILD_PLAT="${BUILD_PLAT}" \
		FCONF_GEN_SRC="${FCONF_GEN_SRC}" \
		FCONF_DTB_FILE="${PLAT_DTB_FILE}" \
		FCONF_GDB_VAR_DUMP="${GDB_VAR_DUMP}" \
		FCONF_GDB_VAR_INCLUDES="${GDB_VAR_INCLUDES}" \
		FCONF_EXTR_PLAT_SOURCES="${FCONF_EXTR_PLAT_SOURCES}" \
		FCONF_EXTR_PLAT_INCLUDE_DIRS="${FCONF_EXTR_PLAT_INCLUDE_DIRS}" \


