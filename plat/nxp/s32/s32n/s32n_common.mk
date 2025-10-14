#
# Copyright 2025 NXP
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

define hexbc
echo "obase=16;ibase=16;$$(echo "$1 $2 $3 $4 $5 $6 $7" | tr 'a-x' 'A-X' | sed 's/0X//g')" | bc
endef

# SoC-specific build parameters
PLAT_COMMON_PATH	:=	plat/nxp/common
PLAT_SOC_PATH		:=	plat/nxp/s32
PLAT_DRIVERS_PATH	:=	drivers/nxp/s32n
PLAT_S32N			:=  ${PLAT_SOC_PATH}/s32n

PLAT_INCLUDES += \
	-Idrivers \
	-Iinclude/common \
	-Iinclude/plat/common \
	-I${PLAT_SOC_PATH}/include \

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

# Enable FSS usage
S32N_USE_FSS ?= 0
$(eval $(call add_define,S32N_USE_FSS))

# Custom BL33 device tree size
BL33_DTB_SIZE ?= 0x5000
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
	lib/cpus/aarch64/cortex_a78_ae.S \
	lib/fconf/fconf.c \
	${PLAT_SOC_PATH}/common/s32_scmi.c \
	${FCONF_GEN_SRC} \

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
	drivers/partition/gpt.c \
	drivers/partition/partition.c \
	drivers/synopsys/ufs/dw_ufs.c \
	drivers/ufs/ufs.c \
	lib/cpus/aarch64/cortex_a78_ae.S \
	${PLAT_DRIVERS_PATH}/ufs/s32n_ufs.c \
	${PLAT_DRIVERS_PATH}/clk/plat_clk.c \
	${XLAT_TABLES_LIB_SRCS} \

BL31_SOURCES += \
	${GICV3_SOURCES} \
	plat/common/plat_gicv3.c \
	plat/common/plat_psci_common.c \
	plat/arm/common/arm_common.c \
	drivers/arm/css/scmi/scmi_pwr_dmn_proto.c \
	drivers/arm/css/scmi/scmi_sys_pwr_proto.c \
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

BL2_ALIGN := 64

DTC_FLAGS		+= -Wno-unit_address_vs_reg

DTC_CPPFLAGS	+= -Iinclude/arch/aarch64

FDT_SOURCES     ?= $(addprefix fdts/, $(patsubst %.dtb,%.dts,$(DTB_FILE_NAME)))
BL2_BIN			:= $(strip $(call IMG_BIN,bl2))
PLAT_DTB_FILE   := ${BUILD_PLAT}/fdts/${DTB_FILE_NAME}

$(eval $(call TOOL_ADD_PAYLOAD,${PLAT_DTB_FILE},--hw-config,${PLAT_DTB_FILE}))

all: fip post_build_checks print_config

post_build_checks: ${PLAT_DTB_FILE}
	$(eval DTB_S = $(shell $(call filesize, ${PLAT_DTB_FILE})))
	$(eval DTB_S_LIMIT = $(shell printf "%d" $(BL33_DTB_SIZE)))
	${Q}if [ $(DTB_S) -gt $(DTB_S_LIMIT) ] ; then \
		echo -e "\n\nError: DTB size ($(DTB_S) bytes) exceeds the BL33_DTB_SIZE limit ($(DTB_S_LIMIT) bytes)."; \
		echo -e "Device tree file: ${PLAT_DTB_FILE}\n\n"; \
		exit 1; \
	fi

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
	${ECHO} "S32N_USE_FSS           = ${S32N_USE_FSS}"
ifeq (${S32N_USE_FSS},1)
	${ECHO} "S32N_SCMI_MBX          = ${S32N_SCMI_MBX}"
else
	${ECHO} "S32N_GIC_MODE          = ${S32N_GIC_MODE}"
endif
	${ECHO} "APS_PERIPH_BASE        = ${APS_PERIPH_BASE}"

	${ECHO} "==================================="

