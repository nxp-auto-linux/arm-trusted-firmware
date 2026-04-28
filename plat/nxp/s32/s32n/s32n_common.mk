#
# Copyright 2025-2026 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#
# S32N SoC Platform Definitions

ARM_ARCH_MAJOR := 8
ARM_ARCH_MINOR := 2

GICV3_SUPPORT_GIC600 := 1

include drivers/arm/gic/v3/gicv3.mk
include lib/libc/libc.mk
include lib/libfdt/libfdt.mk
include lib/xlat_tables_v2/xlat_tables.mk
include drivers/mmc/mmc.mk
include plat/nxp/s32/common/s32_common.mk

# SoC-specific build parameters
PLAT_SOC_PATH		:=	plat/nxp/s32
PLAT_DRIVERS_PATH	:=	drivers/nxp/s32n
PLAT_S32N			:=  ${PLAT_SOC_PATH}/s32n

HAS_DDR_FW		:= 0

include ${PLAT_S32N}/fconf/fconf.mk

PLAT_INCLUDES += \
	-Idrivers \
	-Iinclude/common \
	-Iinclude/${PLAT_DRIVERS_PATH} \
	-Iinclude/lib \
	-Iinclude/plat/common \
	-I${PLAT_SOC_PATH}/include \
	-I${PLAT_S32N}/include \

# Use Linux Kernel Image as BL33, instead of U-boot
S32N_LINUX_KERNEL_AS_BL33	?=	0
$(eval $(call add_define,S32N_LINUX_KERNEL_AS_BL33))

RESET_TO_BL2 := 1
COLD_BOOT_SINGLE_CPU := 0

PROGRAMMABLE_RESET_ADDRESS := 1

# Mandatory for Cortex-A78AE
HW_ASSISTED_COHERENCY := 1
USE_COHERENT_MEM := 0
CTX_INCLUDE_AARCH32_REGS := 0
USE_SPINLOCK_CAS := 1

INIT_UNUSED_NS_EL2	:= 1

# Cortex-A78AE errata workarounds
ERRATA_A78_AE_1941500 := 1
ERRATA_A78_AE_1951502 := 1
ERRATA_A78_AE_2376748 := 1
ERRATA_A78_AE_2395408 := 1

# S32N79 errata workarounds
ERRATA_S32N79_052978 := 1

# Apply SMMU GBPA[SHCFG] config by default
S32N_SMMU_GBPA_GFG ?= 1
$(eval $(call add_define,S32N_SMMU_GBPA_GFG))

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

# Add reserved memory nodes for TF-A stages to the device tree
S32N_DTS_RESERVE_BL_NODES ?= 1
$(eval $(call add_define,S32N_DTS_RESERVE_BL_NODES))

# When set, BL2 acts as Cohort Manager:
# - programs the XRDC AIDA
# - programs the XRDC MIDAC
S32N_BL2_AS_COHORT_MANAGER ?= 1
$(eval $(call add_define,S32N_BL2_AS_COHORT_MANAGER))

# Custom BL33 device tree size
ifeq (${S32N_LINUX_KERNEL_AS_BL33},1)
ifeq ($(BL33_DTB),)
$(error BL33_DTB must be specified when skip-uboot flow is used)
endif

ifeq ($(wildcard $(BL33_DTB)),)
$(error BL33_DTB file not found: $(BL33_DTB))
endif

BL33_DTB_SIZE := $(shell stat -c "%s" ${BL33_DTB})
ifeq ($(BL33_DTB_SIZE),)
$(error Failed to determine BL33_DTB size for: $(BL33_DTB))
endif
else
BL33_DTB_SIZE ?= 0x8000
endif
$(eval $(call add_define,BL33_DTB_SIZE))

# Enable dump of processor register state upon exceptions while running BL31
CRASH_REPORTING		:= 1
# As verbose as it can be
LOG_LEVEL		?= 50

PLAT_BL_COMMON_SOURCES += \
	drivers/arm/css/scmi/scmi_common.c \
	drivers/arm/pl011/aarch64/pl011_console.S	\
	drivers/delay_timer/delay_timer.c \
	drivers/delay_timer/generic_delay_timer.c \
	${FCONF_GEN_SRC} \
	lib/cpus/aarch64/cortex_a78_ae.S \
	lib/fconf/fconf.c \
	${PLAT_SOC_PATH}/common/s32_scmi.c \
	${PLAT_S32N}/plat_helpers.S \
	${PLAT_S32N}/s32n_bl_common.c \
	${PLAT_S32N}/s32n_clk_utils.c \
	${PLAT_S32N}/s32n_fss_scmi.c \
	${PLAT_S32N}/s32n_fss_tlv.c \
	${PLAT_S32N}/s32n_fss_utils.c \
	${PLAT_S32N}/s32n_pl011.c \

BL2_SOURCES += \
	common/desc_image_load.c \
	common/fdt_fixup.c \
	common/tf_crc32.c \
	drivers/clk/clk.c \
	drivers/io/io_block.c \
	drivers/io/io_fip.c \
	drivers/io/io_memmap.c \
	drivers/io/io_storage.c \
	drivers/mmc/mmc.c \
	drivers/nxp/s32/mmc/s32_mmc.c \
	drivers/arm/css/scmi/scmi_clock_proto.c \
	drivers/partition/gpt.c \
	drivers/partition/partition.c \
	drivers/synopsys/ufs/dw_ufs.c \
	drivers/arm/smmu/smmu_v3.c \
	drivers/ufs/ufs.c \
	lib/cpus/aarch64/cortex_a78_ae.S \
	${PLAT_DRIVERS_PATH}/clk/plat_clk.c \
	${PLAT_DRIVERS_PATH}/ufs/s32n_ufs.c \
	${PLAT_DRIVERS_PATH}/cohort_manager/cohort_manager.c \
	${PLAT_DRIVERS_PATH}/xrdc_aida/xrdc_aida.c \
	${PLAT_DRIVERS_PATH}/xrdc_common/xrdc_common.c \
	${PLAT_DRIVERS_PATH}/xrdc_midac/xrdc_midac.c \
	${PLAT_S32N}/s32n_bl2_el3_setup.c \
	${PLAT_S32N}/s32n_plat_funcs.c \
	${PLAT_S32N}/s32n_io_storage.c \
	${XLAT_TABLES_LIB_SRCS} \
	${TBBR_SOURCES} \

BL31_SOURCES += \
	${GICV3_SOURCES} \
	plat/common/plat_gicv3.c \
	plat/common/plat_psci_common.c \
	plat/arm/common/arm_common.c \
	drivers/arm/css/scmi/scmi_base_proto.c \
	drivers/arm/css/scmi/scmi_pwr_dmn_proto.c \
	drivers/arm/css/scmi/scmi_sys_pwr_proto.c \
	${PLAT_SOC_PATH}/common/s32_irq_mgmt.c \
	${PLAT_S32N}/s32n_aps_mru.c \
	${PLAT_S32N}/s32n_bl31_setup.c \
	${PLAT_S32N}/s32n_bl31_trampoline.S \
	${PLAT_S32N}/s32n_plat_funcs.c \
	${PLAT_S32N}/s32n_psci.c \
	${PLAT_S32N}/s32n_svc.c \
	${PLAT_S32N}/s32n_topology.c \
	${XLAT_TABLES_LIB_SRCS} \

# Include HSE2 interface headers
ifneq (${NXP_HSE_FWDIR},)
PLAT_INCLUDES += -I${NXP_HSE_FWDIR}/interface \
                 -I${NXP_HSE_FWDIR}/interface/common \
                 -I${NXP_HSE_FWDIR}/interface/services \
                 -I${NXP_HSE_FWDIR}/interface/config

BL2_SOURCES += ${PLAT_DRIVERS_PATH}/hse2/hse_mu.c \
               ${PLAT_DRIVERS_PATH}/hse2/hse_rmem.c \
               ${PLAT_DRIVERS_PATH}/hse2/hse_core.c
endif

### Devel & Debug options ###
ifeq (${DEBUG},1)
	CFLAGS			+= -O0
else
	CFLAGS			+= -Os
endif

# Tools
DD ?= dd status=none conv=fdatasync

define filesize
printf "%d" $$(stat -c "%s" $1)
endef

define hexfilesize
printf "0x%x" $$(stat -c "%s" $1)
endef

DTC_FLAGS		+= -Wno-unit_address_vs_reg

DTC_CPPFLAGS	+= -Iinclude/arch/aarch64

FDT_SOURCES     ?= $(addprefix fdts/, $(patsubst %.dtb,%.dts,$(DTB_FILE_NAME)))
BL2_BIN			:= $(strip $(call IMG_BIN,bl2))
PLAT_DTB_FILE   := ${BUILD_PLAT}/fdts/${DTB_FILE_NAME}
FCONF_GEN_SRC	:= ${BUILD_PLAT}/fconf_extractor_settings.c

ifeq (${S32N_LINUX_KERNEL_AS_BL33},1)
$(eval $(call TOOL_ADD_PAYLOAD,${BL33_DTB},--nt-fw-config))
else
$(eval $(call TOOL_ADD_PAYLOAD,${PLAT_DTB_FILE},--nt-fw-config,${PLAT_DTB_FILE}))
endif

FCONF_EXTRACTOR_BIN := ${BUILD_PLAT}/tools/fconf_extractor/fconf_extractor

ifeq (${SECBOOT_SUPPORT},1)
all: sign_image fip post_build_checks print_config
else
all: fip post_build_checks print_config
endif

ifneq (${S32N_LINUX_KERNEL_AS_BL33},1)
post_build_checks: ${PLAT_DTB_FILE}
	$(eval DTB_S = $(shell $(call filesize, ${PLAT_DTB_FILE})))
	$(eval DTB_S_LIMIT = $(shell printf "%d" $(BL33_DTB_SIZE)))
	${Q}if [ $(DTB_S) -gt $(DTB_S_LIMIT) ] ; then \
		echo -e "\n\nError: DTB size ($(DTB_S) bytes) exceeds the BL33_DTB_SIZE limit ($(DTB_S_LIMIT) bytes)."; \
		echo -e "Device tree file: ${PLAT_DTB_FILE}\n\n"; \
		exit 1; \
	fi
else
post_build_checks: ;
endif

print_config: fip
	${ECHO} "==================================="
	${ECHO} "	Build Configuration	"
	${ECHO} "==================================="

	${ECHO} "PLATFORM_CLUSTER_COUNT = ${PLATFORM_CLUSTER_COUNT}"
	${ECHO} "S32N_PLAT_PRIMARY_CPU  = ${S32N_PLAT_PRIMARY_CPU}"
	${ECHO} "S32N_PL011_MODULE      = ${S32N_PL011_MODULE}"
	${ECHO} "S32N_BOOT_SRC          = ${S32N_BOOT_SRC}"
	${ECHO} "DDR_START              = ${DDR_START}"
	${ECHO} "DDR_END                = ${DDR_END}"
	${ECHO} "PLAT_BL_MEM_SIZE       = ${PLAT_BL_MEM_SIZE}"
	${ECHO} "BL33_ENTRYPOINT        = ${BL33_ENTRYPOINT}"
	${ECHO} "BL33_DTB_SIZE          = ${BL33_DTB_SIZE}"
	${ECHO} "S32N_LINUX_KERNEL_AS_BL33 = ${S32N_LINUX_KERNEL_AS_BL33}"
	${ECHO} "S32N_DTS_RESERVE_BL_NODES = ${S32N_DTS_RESERVE_BL_NODES}"
	${ECHO} "APS_PERIPH_BASE        = ${APS_PERIPH_BASE}"

	${ECHO} "==================================="

GDB_VAR_DUMP := \
	clk_config \
	gic_config \
	hse_config \
	dt_memory_regions \
	scmi_doorbell_rx_config \
	scmi_doorbell_tx_config \
	scmi_ospm_ch_config \
	scmi_fss_ch_rx_config \
	scmi_fss_ch_tx_config \
	scmi_aps_mru_config \
	scmi_sh_mem_config \
	smmu_config \
	tlv_config \
	ufs_config \
	xrdc_aida_config \

GDB_VAR_INCLUDES := \
	s32n_fconf_clk_getter.h \
	s32n_fconf_gic_getter.h \
	s32n_fconf_hse_getter.h \
	s32n_fconf_memory_getter.h \
	s32n_fconf_scmi_getter.h \
	s32n_fconf_smmu_getter.h \
	s32n_fconf_tlv_getter.h \
	s32n_fconf_ufs_getter.h \
	s32n_fconf_xrdc_aida_getter.h \

FCONF_EXTRACTOR_SRC_DIR = "tools/nxp/fconf_extractor"

FCONF_SRCS_PLAT_DIR := ${PLAT_S32N}/fconf

FCONF_EXTR_PLAT_SOURCES := \
	${FCONF_SRCS_PLAT_DIR}/plat_fconf_extractor.c \
	${FCONF_SRCS_PLAT_DIR}/s32n_fconf_clk_getter.c \
	${FCONF_SRCS_PLAT_DIR}/s32n_fconf_gic_getter.c \
	${FCONF_SRCS_PLAT_DIR}/s32n_fconf_hse_getter.c \
	${FCONF_SRCS_PLAT_DIR}/s32n_fconf_memory_getter.c \
	${FCONF_SRCS_PLAT_DIR}/s32n_fconf_scmi_getter.c \
	${FCONF_SRCS_PLAT_DIR}/s32n_fconf_smmu_getter.c \
	${FCONF_SRCS_PLAT_DIR}/s32n_fconf_tlv_getter.c \
	${FCONF_SRCS_PLAT_DIR}/s32n_fconf_ufs_getter.c \
	${FCONF_SRCS_PLAT_DIR}/s32n_fconf_utils.c \
	${FCONF_SRCS_PLAT_DIR}/s32n_fconf_xrdc_aida_getter.c \

FCONF_EXTR_PLAT_INCLUDE_DIRS := \
	${PLAT_S32N}/include/fconf \
	${PLAT_S32N}/include \

${FCONF_GEN_SRC}: ${PLAT_DTB_FILE}
	${Q}${MAKE} --no-print-directory -C "${FCONF_EXTRACTOR_SRC_DIR}" \
		BUILD_PLAT="${BUILD_PLAT}" \
		FCONF_GEN_SRC="${FCONF_GEN_SRC}" \
		FCONF_DTB_FILE="${PLAT_DTB_FILE}" \
		FCONF_GDB_VAR_DUMP="${GDB_VAR_DUMP}" \
		FCONF_GDB_VAR_INCLUDES="${GDB_VAR_INCLUDES}" \
		FCONF_EXTR_PLAT_SOURCES="${FCONF_EXTR_PLAT_SOURCES}" \
		FCONF_EXTR_PLAT_INCLUDE_DIRS="${FCONF_EXTR_PLAT_INCLUDE_DIRS}" \


ifeq (${SECBOOT_SUPPORT},1)
include plat/nxp/s32/s32n/tbbr/s32n_hse_secboot.mk
endif
