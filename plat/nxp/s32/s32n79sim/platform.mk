#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

ARM_ARCH_MAJOR := 8
ARM_ARCH_MINOR := 2

GICV3_SUPPORT_GIC600 := 1

include drivers/arm/gic/v3/gicv3.mk
include lib/libc/libc.mk
include lib/libfdt/libfdt.mk
include lib/xlat_tables_v2/xlat_tables.mk

# SoC-specific build parameters
PLAT_COMMON_PATH	:=	plat/nxp/common
PLAT_SOC_PATH		:=	plat/nxp/s32
PLAT_DRIVERS_PATH	:=	drivers/nxp/s32n
PLAT_S32N79SIM		:=  ${PLAT_SOC_PATH}/s32n79sim

PLAT_INCLUDES += \
	-Idrivers \
	-Iinclude/common \
	-Iinclude/plat/common \
	-I${PLAT_SOC_PATH}/include \
	-I${PLAT_S32N79SIM}/include \
	-I${PLAT_SOC_PATH}/include \

S32N_SIM := 1
$(eval $(call add_define,S32N_SIM))

RESET_TO_BL2 := 1
COLD_BOOT_SINGLE_CPU := 0

PROGRAMMABLE_RESET_ADDRESS := 1

# Mandatory for Cortex-A78AE
HW_ASSISTED_COHERENCY := 1
USE_COHERENT_MEM := 0
CTX_INCLUDE_AARCH32_REGS := 0

INIT_UNUSED_NS_EL2	:= 1

# Cortex-A78AE errata workarounds
ERRATA_A78_AE_1941500 := 1
ERRATA_A78_AE_1951502 := 1
ERRATA_A78_AE_2376748 := 1
ERRATA_A78_AE_2395408 := 1

# Default S32N APS topology
PLATFORM_CLUSTER_COUNT	?= 1
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

# U-Boot: CONFIG_SYS_DATA_BASE
BL33_IMG_BASE ?= 0xFF700000
$(eval $(call add_define,BL33_BASE))
# U-Boot: CONFIG_SYS_TEXT_BASE
BL33_ENTRYPOINT ?= 0xFF7A0000
$(eval $(call add_define,BL33_ENTRYPOINT))

# Boot source selection.
S32N_DDR_BOOT_SOURCE = 0
S32N_UFS_BOOT_SOURCE = 0
S32N_MMC_BOOT_SOURCE = 0

# The default boot storage for S32N platforms is the UFS.
# Possible boot source options: ddr, ufs, mmc.
S32N_BOOT_SRC ?= ufs

ifeq ($(S32N_BOOT_SRC), ddr)
	S32N_DDR_BOOT_SOURCE = 1
else ifeq ($(S32N_BOOT_SRC), ufs)
	PLAT_PARTITION_BLOCK_SIZE := 4096
	S32N_UFS_BOOT_SOURCE = 1
$(eval $(call add_define,PLAT_PARTITION_BLOCK_SIZE))
else ifeq ($(S32N_BOOT_SRC), mmc)
	S32N_MMC_BOOT_SOURCE = 1
else
$(error Unknown boot source $(S32N_BOOT_SRC). Please choose between: ufs, ddr, mmc)
endif

$(eval $(call add_define,S32N_DDR_BOOT_SOURCE))
$(eval $(call add_define,S32N_UFS_BOOT_SOURCE))
$(eval $(call add_define,S32N_MMC_BOOT_SOURCE))

PLAT_PARTITION_MAX_ENTRIES := 3
$(eval $(call add_define,PLAT_PARTITION_MAX_ENTRIES))

# The location in DDR and the size of the FIP image when booting from DDR.
S32N_FIP_BASE ?= 0x80000000
$(eval $(call add_define,S32N_FIP_BASE))
S32N_FIP_SIZE ?= 0x100000
$(eval $(call add_define,S32N_FIP_SIZE))

# Choose which PL011 module to use
S32N_PL011_MODULE ?= 0
$(eval $(call add_define,S32N_PL011_MODULE))

# Enable FSS usage
S32N_USE_FSS ?= 0
$(eval $(call add_define,S32N_USE_FSS))

# Choose the SCMI APS-FSS mailbox number and size
S32N_SCMI_MBX ?= 0
$(eval $(call add_define,S32N_SCMI_MBX))

# Enable dump of processor register state upon exceptions while running BL31
CRASH_REPORTING		:= 1
# As verbose as it can be
LOG_LEVEL		?= 50

PLAT_BL_COMMON_SOURCES += \
	drivers/arm/pl011/aarch64/pl011_console.S	\
	drivers/delay_timer/delay_timer.c \
	drivers/delay_timer/generic_delay_timer.c \
	lib/cpus/aarch64/cortex_a78_ae.S \
	lib/fconf/fconf.c \
	${PLAT_SOC_PATH}/common/s32_scmi.c \
	${PLAT_S32N79SIM}/plat_helpers.S \
	${PLAT_S32N79SIM}/s32n_bl_common.c \
	${PLAT_S32N79SIM}/s32n_fconf_scmi_getter.c \
	${PLAT_S32N79SIM}/s32n_fss_scmi.c \
	${BOOT_INFO_SRC} \

BL2_SOURCES += \
	common/desc_image_load.c \
	common/fdt_fixup.c \
	common/tf_crc32.c \
	drivers/io/io_block.c \
	drivers/io/io_fip.c \
	drivers/io/io_memmap.c \
	drivers/io/io_storage.c \
	drivers/mmc/mmc.c \
	drivers/nxp/s32/mmc/s32_mmc.c \
	drivers/partition/gpt.c \
	drivers/partition/partition.c \
	drivers/synopsys/ufs/dw_ufs.c \
	drivers/ufs/ufs.c \
	lib/cpus/aarch64/cortex_a78_ae.S \
	plat/nxp/s32/common/s32_bl2_common.c \
	${PLAT_DRIVERS_PATH}/ufs/s32n_ufs.c \
	${PLAT_S32N79SIM}/s32n_bl_common.c \
	${PLAT_S32N79SIM}/s32n_bl2_el3_setup.c \
	${PLAT_S32N79SIM}/s32n_bl2_mem_params_desc.c \
	${PLAT_S32N79SIM}/s32n_io_storage.c \
	${XLAT_TABLES_LIB_SRCS} \

BL31_SOURCES += \
	${GICV3_SOURCES} \
	plat/common/plat_gicv3.c \
	plat/common/plat_psci_common.c \
	plat/arm/common/arm_common.c \
	drivers/arm/css/scmi/scmi_common.c \
	drivers/arm/css/scmi/scmi_ap_core_proto.c \
	${PLAT_S32N79SIM}/s32n_bl31_setup.c \
	${PLAT_S32N79SIM}/s32n_psci.c \
	${PLAT_S32N79SIM}/s32n_svc.c \
	${PLAT_S32N79SIM}/s32n_topology.c \
	${XLAT_TABLES_LIB_SRCS} \

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

define hexbc
echo "obase=16;ibase=16;$$(echo "$1 $2 $3 $4 $5 $6 $7" | tr 'a-x' 'A-X' | sed 's/0X//g')" | bc
endef

BL2_ALIGN := 64

DTC_FLAGS		+= -Wno-unit_address_vs_reg

DTC_CPPFLAGS	+= -Iinclude/arch/aarch64

DTB_FILE_NAME   ?= s32n79-simulator.dtb
FDT_SOURCES     := $(addprefix fdts/, $(patsubst %.dtb,%.dts,$(DTB_FILE_NAME)))
BL2_W_DTB		:= ${BUILD_PLAT}/bl2_w_dtb.bin
BL2_BIN			:= $(strip $(call IMG_BIN,bl2))
PLAT_DTB_FILE   := ${BUILD_PLAT}/fdts/${DTB_FILE_NAME}
BOOT_INFO_SRC   := ${BUILD_PLAT}/boot_info.c
DTB_SIZE_FILE    = ${BUILD_PLAT}/dtb_size

all: fip ${BL2_W_DTB}

${DTB_SIZE_FILE}: dtbs
	${ECHO} "  CREATE  $@"
	$(eval BL2_ALIGN_HEX = $(shell printf "0x%x" ${BL2_ALIGN}))
	$(eval DTB_S_HEX = $(shell $(call hexfilesize, ${PLAT_DTB_FILE})))
	$(eval DTB_SIZE = 0x$(shell $(call hexbc, ${DTB_S_HEX}, /, ${BL2_ALIGN_HEX}, *, ${BL2_ALIGN_HEX}, +, ${BL2_ALIGN_HEX})))
	${Q}echo "${DTB_SIZE}" > $@

${BOOT_INFO_SRC}: ${DTB_SIZE_FILE}
	${ECHO} "  CREATE  $@"
	${Q}echo "const unsigned int dtb_size = $$(cat ${DTB_SIZE_FILE});" > ${BOOT_INFO_SRC}

${BL2_W_DTB}: bl2 dtbs ${DTB_SIZE_FILE}

	$(eval BL2_BIN_S = $(shell $(call filesize, ${BL2_BIN})))
	$(eval DTB_NO_ALIGN_S = $(shell $(call filesize, ${PLAT_DTB_FILE})))
	$(eval DTB_SIZE = $(shell cat ${DTB_SIZE_FILE}))

	@${DD} if=/dev/zero of=$@ bs=$$(printf "%d" ${DTB_SIZE}) count=1
	@${DD} if=${PLAT_DTB_FILE} of=$@ bs=${DTB_NO_ALIGN_S} count=1
	@${DD} if=${BL2_BIN} of=$@ seek=$$(printf "%d" ${DTB_SIZE}) oflag=seek_bytes

	${ECHO} "BL2 DTB file size=${DTB_SIZE}"
	${ECHO} "Generated ${BL2_W_DTB} successfully."

	${ECHO} "==================================="
	${ECHO} "	Build Configuration	"
	${ECHO} "==================================="

	${ECHO} "PLATFORM_CLUSTER_COUNT = ${PLATFORM_CLUSTER_COUNT}"
	${ECHO} "S32N_PLAT_PRIMARY_CPU  = ${S32N_PLAT_PRIMARY_CPU}"
	${ECHO} "S32N_PL011_MODULE      = ${S32N_PL011_MODULE}"
	${ECHO} "S32N_BOOT_SRC          = ${S32N_BOOT_SRC}"
	${ECHO} "S32N_BL33_AT_EL2       = ${S32N_BL33_AT_EL2}"
	${ECHO} "DDR_START              = ${DDR_START}"
	${ECHO} "DDR_END                = ${DDR_END}"
	${ECHO} "PLAT_BL_MEM_SIZE       = ${PLAT_BL_MEM_SIZE}"
	${ECHO} "BL33_IMG_BASE          = ${BL33_IMG_BASE}"
	${ECHO} "BL33_ENTRYPOINT        = ${BL33_ENTRYPOINT}"
	${ECHO} "S32N_USE_FSS           = ${S32N_USE_FSS}"
ifeq (${S32N_USE_FSS},1)
	${ECHO} "S32N_SCMI_MBX          = ${S32N_SCMI_MBX}"
endif

	${ECHO} "==================================="
