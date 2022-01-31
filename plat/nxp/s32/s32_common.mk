#
# Copyright 2021-2022 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

include drivers/arm/gic/v3/gicv3.mk
include lib/libc/libc.mk
include lib/libfdt/libfdt.mk
include lib/xlat_tables_v2/xlat_tables.mk
include make_helpers/build_macros.mk

ERRATA_A53_855873	:= 1
ERRATA_A53_836870	:= 1
ERRATA_A53_1530924	:= 1
ERRATA_SPECULATIVE_AT	:= 1

# Tools
HEXDUMP ?= xxd
SED ?= sed

S32G_EMU		?= 0
$(eval $(call add_define_val,S32G_EMU,$(S32G_EMU)))

S32GEN1_DRAM_INLINE_ECC	?= 1
$(eval $(call add_define_val,S32GEN1_DRAM_INLINE_ECC,$(S32GEN1_DRAM_INLINE_ECC)))

DDR_DRV = drivers/nxp/s32/ddr

DDR_DRV_SRCS += \
	${DDR_DRV}/ddr_density.c \
	${DDR_DRV}/ddr_init.c \
	${DDR_DRV}/ddr_utils_mmio.c \
	${DDR_DRV}/ddr_lp_mmio.c \
	${DDR_DRV}/ddr_lp_csr.c \
	${DDR_DRV}/ddrss_cfg.c \
	${DDR_DRV}/imem_cfg.c \

BL2_AT_EL3		:= 1

PLAT_INCLUDES 	+= \
			-Idrivers \
			-Iinclude/common/tbbr \
			-Iinclude/drivers \
			-Iinclude/drivers/nxp/s32 \
			-Iinclude/lib \
			-Iinclude/lib/libc \
			-Iinclude/lib/psci \
			-Iinclude/plat/arm/common \
			-Iinclude/plat/arm/soc/common \
			-Iinclude/plat/common \
			-Iplat/nxp/s32/include \

PLAT_BL_COMMON_SOURCES += \
			${GICV3_SOURCES} \
			common/fdt_wrappers.c \
			drivers/nxp/uart/linflexuart.c \
			plat/nxp/s32/s32_bl_common.c \
			plat/nxp/s32/s32_dt.c \
			plat/nxp/s32/s32_lowlevel_common.S \
			plat/nxp/s32/s32_sramc.c \
			plat/nxp/s32/s32_sramc_asm.S \
			plat/nxp/s32/s32_linflexuart.c \
			plat/nxp/s32/s32_linflexuart_crash.S \
			plat/nxp/s32/s32_mc_me.c \
			plat/nxp/s32/s32_ncore.c \
			plat/nxp/s32/s32_pinctrl.c \
			drivers/delay_timer/delay_timer.c \
			drivers/delay_timer/generic_delay_timer.c \
			drivers/nxp/s32/memory_pool.c \
			drivers/nxp/s32/clk/early_clocks.c \
			drivers/nxp/s32/clk/enable_clk.c \
			drivers/nxp/s32/clk/get_rate.c \
			drivers/nxp/s32/clk/plat_clk.c \
			drivers/nxp/s32/clk/s32gen1_clk.c \
			drivers/nxp/s32/rst/s32gen1_rst.c \
			drivers/nxp/s32/clk/set_par_rate.c \
			drivers/nxp/s32/i2c/s32_i2c.c \
			${FIP_INFO_SRC} \

BL2_SOURCES += \
			${XLAT_TABLES_LIB_SRCS} \
			${DDR_DRV_SRCS} \
			common/desc_image_load.c \
			common/fdt_fixup.c \
			drivers/io/io_fip.c \
			drivers/io/io_storage.c \
			drivers/mmc/mmc.c \
			drivers/nxp/s32/io/io_mmc.c \
			drivers/nxp/s32/io/io_memmap.c \
			drivers/nxp/s32/mmc/s32_mmc.c \
			lib/optee/optee_utils.c \
			plat/nxp/s32/s32_bl2_el3.c \
			plat/nxp/s32/s32_storage.c \
			plat/nxp/s32/s32_lowlevel_bl2.S \

BL31_SOURCES += \
			${XLAT_TABLES_LIB_SRCS} \
			drivers/scmi-msg/base.c \
			drivers/scmi-msg/clock.c \
			drivers/scmi-msg/entry.c \
			drivers/scmi-msg/reset_domain.c \
			drivers/nxp/s32/clk/clk.c \
			drivers/nxp/s32/clk/fixed_clk.c \
			drivers/nxp/s32/clk/s32gen1_scmi_clk.c \
			drivers/nxp/s32/clk/s32gen1_scmi_ids.c \
			plat/common/plat_gicv3.c \
			plat/common/plat_psci_common.c \
			plat/nxp/s32/include/plat_macros.S \
			plat/nxp/s32/s32_bl31.c \
			plat/nxp/s32/s32_lowlevel_bl31.S \
			plat/nxp/s32/s32_scmi_clk.c \
			plat/nxp/s32/s32_scmi_rst.c \
			plat/nxp/s32/s32_svc.c \
			plat/nxp/s32/s32_psci.c \

DTC_FLAGS		+= -Wno-unit_address_vs_reg

all: check_dtc_version
check_dtc_version:
	$(eval DTC_VERSION_RAW = $(shell $(DTC) --version | cut -f3 -d" " \
							  | cut -f1 -d"-"))
	$(eval DTC_VERSION = $(shell echo $(DTC_VERSION_RAW) | sed "s/\./0/g"))
	@if [ ${DTC_VERSION} -lt 10406 ]; then \
		echo "$(DTC) version must be 1.4.6 or above"; \
		false; \
	fi

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

BL2_EL3_STACK_ALIGNMENT :=	512
$(eval $(call add_define_val,BL2_EL3_STACK_ALIGNMENT,$(BL2_EL3_STACK_ALIGNMENT)))

FDT_SOURCES             = $(addprefix fdts/, $(patsubst %.dtb,%.dts,$(DTB_FILE_NAME)))

### Devel & Debug options ###
ifeq (${DEBUG},1)
	CFLAGS			+= -O0
else
	CFLAGS			+= -Os
endif
# Enable dump of processor register state upon exceptions while running BL31
CRASH_REPORTING		:= 1
# As verbose as it can be
LOG_LEVEL		?= 50

# Sharing the LinFlexD UART is not always a safe option. Different drivers
# (e.g. Linux and TF-A) can configure the UART controller differently; even so,
# there is no hardware lock to prevent concurrent access to the device. For now,
# opt to suppress output (except for crash reporting). For debugging and other
# similarly safe contexts, output can be turned back on using this switch.
S32_USE_LINFLEX_IN_BL31	?= 0
$(eval $(call add_define_val,S32_USE_LINFLEX_IN_BL31,$(S32_USE_LINFLEX_IN_BL31)))

# Whether we're going to run a hypervisor (EL2) or jump straight into the
# bootloader (EL1)
S32_HAS_HV		?= 0
$(eval $(call add_define_val,S32_HAS_HV,$(S32_HAS_HV)))

# Reserve some space at the end of SRAM for external apps and include it
# in the calculation of FIP_BASE address.
EXT_APP_SIZE		:= 0x100000
$(eval $(call add_define,EXT_APP_SIZE))
FIP_MAXIMUM_SIZE	:= 0x300000
$(eval $(call add_define,FIP_MAXIMUM_SIZE))
# FIP offset from the far end of SRAM; leave it to the C code to perform
# the arithmetic
FIP_ROFFSET		:= "(EXT_APP_SIZE + FIP_MAXIMUM_SIZE)"
$(eval $(call add_define,FIP_ROFFSET))

BL2_W_DTB		:= ${BUILD_PLAT}/bl2_w_dtb.bin
all: ${BL2_W_DTB}
# WARNING: 1024 * 8 = DTB_SIZE. These two should be kept in sync
${BL2_W_DTB}: bl2 dtbs
	@cp ${BUILD_PLAT}/fdts/${DTB_FILE_NAME} $@
	@dd if=${BUILD_PLAT}/bl2.bin of=$@ bs=1024 seek=8 status=none

ifeq ($(MKIMAGE),)
BL33DIR = $(shell dirname $(BL33))
MKIMAGE = $(BL33DIR)/tools/mkimage
endif
MKIMAGE_CFG ?= ${BL33DIR}/u-boot.cfgout

DUMMY_STAGE := ${BUILD_PLAT}/dummy_fip_stage
DUMMY_FIP := ${BUILD_PLAT}/dummy_fip
FIP_HDR_SIZE_FILE := ${BUILD_PLAT}/fip_hdr_size
FIP_INFO_SRC := ${BUILD_PLAT}/fip_info.c
FIP_OFFSET_FILE = ${BUILD_PLAT}/fip_offset
IVT_LOCATION_FILE = ${BUILD_PLAT}/ivt_location
FIP_SD_OFFSET_FILE = ${BUILD_PLAT}/fip_sd_offset_flag
FIP_EMMC_OFFSET_FILE = ${BUILD_PLAT}/fip_emmc_offset_flag
FIP_QSPI_OFFSET_FILE = ${BUILD_PLAT}/fip_qspi_offset_flag
FIP_MEMORY_OFFSET_FILE = ${BUILD_PLAT}/fip_mem_offset_flag
DUMMY_FIP_S32 = ${BUILD_PLAT}/dummy_fip.s32
MKIMAGE_FIP_CONF_FILE = ${BUILD_PLAT}/fip.cfgout
BL2_W_DTB_SIZE_FILE = ${BUILD_PLAT}/bl2_w_dtb_size

define hexbc
echo "obase=16;ibase=16;$$(echo "$1 $2 $3 $4 $5" | tr 'a-x' 'A-X' | sed 's/0X//g')" | bc
endef

define update_fip
${FIPTOOL} update --align ${FIP_ALIGN} --tb-fw $1 --soc-fw-config $2 $3
endef

define get_fip_hdr_size
printf "0x%x" $$(${FIPTOOL} info $1 | awk -F'[=,]' '{print strtonum($$2)}' | sort -n | head -n1)
endef

define get_bl2_size
${FIPTOOL} info $1 | grep BL2 | sed 's/.*size=\([^,]\+\).*/\1/g'
endef

# Execute mkimage
# $1 - Entry point
# $2 - Load address
# $3 - Configuration file
# $4 - Input file
# $5 - Output file
define run_mkimage
cd ${BL33DIR} && \
	${MKIMAGE} \
	-e $1 -a $2 -T s32gen1image \
	-n $3 -d $4 $5
endef

${DUMMY_STAGE}: | ${BUILD_PLAT}
	${Q}${ECHO} "  TOUCH   $@"
	${Q}touch $@

# Replace all fiptool args with a dummy state except '--align' parameter
${DUMMY_FIP}: fiptool ${DUMMY_STAGE} | ${BUILD_PLAT}
	${Q}${ECHO} "  FIP     $@"
	${Q}ARGS=$$(echo "${FIP_ARGS}" | sed "s#\(--[^ a]\+\)\s\+\([^ ]\+\)#\1 ${DUMMY_STAGE}#g"); \
		${FIPTOOL} create $${ARGS} "$@_temp"
	${Q}$(call update_fip, ${DUMMY_STAGE}, ${DUMMY_STAGE}, "$@_temp")
	${Q}mv "$@_temp" $@

${FIP_HDR_SIZE_FILE}: ${DUMMY_FIP} ${FIPTOOL}
	${Q}${ECHO} "  CREATE  $@"
	${Q}$(call get_fip_hdr_size, ${DUMMY_FIP}) > $@

${DUMMY_FIP_S32}: ${DUMMY_FIP}
	${Q}${ECHO} "  MKIMAGE $@"
	${Q}$(call run_mkimage, ${BL2_BASE}, ${BL2_BASE}, ${MKIMAGE_CFG}, $<, $@) 2> /dev/null

${IVT_LOCATION_FILE}: ${DUMMY_FIP_S32}
	${Q}${ECHO} "  MKIMAGE $@"
	${Q}${MKIMAGE} -l $< 2>&1 | grep 'IVT Location' | awk -F':' '{print $$2}' | xargs > $@

FIP_OFFSET_DELTA ?= 0

define save_fip_off
	FIP_OFFSET=0x$$($(call hexbc, $1, +, ${FIP_OFFSET_DELTA})); \
	echo "$${FIP_OFFSET}" > "$2"
endef

${FIP_OFFSET_FILE}: ${DUMMY_FIP_S32}
	${Q}${ECHO} "  MKIMAGE $@"
	${Q}OFF=$$(${MKIMAGE} -l $< 2>&1 | grep Application | awk '{print $$3}');\
	$(call save_fip_off, $${OFF},$@)

ifeq ($(FIP_SD_OFFSET)$(FIP_QSPI_OFFSET)$(FIP_MEMORY_OFFSET)$(FIP_EMMC_OFFSET),)
${FIP_SD_OFFSET_FILE}: ${IVT_LOCATION_FILE} ${FIP_OFFSET_FILE}
	${Q}${ECHO} "  CREATE  $@"
	${Q}[ "$$(cat "${IVT_LOCATION_FILE}")" = "QSPI" ] && echo "0" > "$@" || cat "${FIP_OFFSET_FILE}" > "$@"

${FIP_QSPI_OFFSET_FILE}: ${IVT_LOCATION_FILE} ${FIP_OFFSET_FILE}
	${Q}${ECHO} "  CREATE  $@"
	${Q}[ "$$(cat "${IVT_LOCATION_FILE}")" = "QSPI" ] && cat "${FIP_OFFSET_FILE}" > "$@" || echo "0" > "$@"

# Cannot determine if it's an eMMC boot based on mkimage output
${FIP_EMMC_OFFSET_FILE}: FORCE
	${Q}${ECHO} "  CREATE  $@"
	${Q}${ECHO} "0" > "$@"

${FIP_MEMORY_OFFSET_FILE}: FORCE
	${Q}${ECHO} "  CREATE  $@"
	${Q}${ECHO} "0" > "$@"
else
ifdef FIP_SD_OFFSET
STORAGE_LOCATIONS = 1
endif
ifdef FIP_EMMC_OFFSET
STORAGE_LOCATIONS := $(STORAGE_LOCATIONS)1
endif
ifdef FIP_QSPI_OFFSET
STORAGE_LOCATIONS := $(STORAGE_LOCATIONS)1
endif
ifdef FIP_MEMORY_OFFSET
STORAGE_LOCATIONS := $(STORAGE_LOCATIONS)1
endif

ifneq ($(STORAGE_LOCATIONS),1)
$(error "Multiple FIP storage locations were found.")
endif

FIP_SD_OFFSET ?= 0
FIP_EMMC_OFFSET ?= 0
FIP_QSPI_OFFSET ?= 0
FIP_MEMORY_OFFSET ?= 0

${FIP_SD_OFFSET_FILE}: FORCE
	${Q}${ECHO} "  CREATE  $@"
	${Q}${ECHO} "${FIP_SD_OFFSET}" > "$@"

${FIP_EMMC_OFFSET_FILE}: FORCE
	${Q}${ECHO} "  CREATE  $@"
	${Q}${ECHO} "${FIP_EMMC_OFFSET}" > "$@"

${FIP_QSPI_OFFSET_FILE}: FORCE
	${Q}${ECHO} "  CREATE  $@"
	${Q}${ECHO} "${FIP_QSPI_OFFSET}" > "$@"

${FIP_MEMORY_OFFSET_FILE}: FORCE
	${Q}${ECHO} "  CREATE  $@"
	${Q}${ECHO} "${FIP_MEMORY_OFFSET}" > "$@"
endif

${FIP_INFO_SRC}: ${FIP_SD_OFFSET_FILE} ${FIP_EMMC_OFFSET_FILE} ${FIP_QSPI_OFFSET_FILE} ${FIP_MEMORY_OFFSET_FILE}
	${Q}${ECHO} "  CREATE  $@"
	${Q}${ECHO} "const unsigned long fip_sd_offset = $$(cat ${FIP_SD_OFFSET_FILE});" > ${FIP_INFO_SRC}
	${Q}${ECHO} "const unsigned long fip_emmc_offset = $$(cat ${FIP_EMMC_OFFSET_FILE});" >> ${FIP_INFO_SRC}
	${Q}${ECHO} "const unsigned long fip_qspi_offset = $$(cat ${FIP_QSPI_OFFSET_FILE});" >> ${FIP_INFO_SRC}
	${Q}${ECHO} "const unsigned long fip_mem_offset = $$(cat ${FIP_MEMORY_OFFSET_FILE});" >> ${FIP_INFO_SRC}

${BL2_W_DTB_SIZE_FILE}: ${BL2_W_DTB}
	${Q}${ECHO} "  CREATE  $@"
	${Q}printf "0x%x" $$(stat -c "%s" $<) > $@

${MKIMAGE_FIP_CONF_FILE}: ${BL2_W_DTB_SIZE_FILE} ${FIP_HDR_SIZE_FILE} FORCE
	${Q}${ECHO} "  CREATE  $@"
	${Q}cp -f ${MKIMAGE_CFG} $@
	${Q}BL2_W_DTB_SIZE=$$(cat ${BL2_W_DTB_SIZE_FILE}); \
	HDR_SIZE=$$(cat ${FIP_HDR_SIZE_FILE}); \
	T_SIZE=0x$$($(call hexbc, $${BL2_W_DTB_SIZE}, +, $${HDR_SIZE})); \
	echo "DATA_FILE SIZE $$T_SIZE" >> $@

FIP_ALIGN := 8
all: add_to_fip
add_to_fip: fip ${BL2_W_DTB}
	$(eval FIP_MAXIMUM_SIZE_10 = $(shell printf "%d\n" ${FIP_MAXIMUM_SIZE}))
	${Q}$(call update_fip, ${BUILD_PLAT}/bl2_w_dtb.bin, ${BUILD_PLAT}/fdts/${DTB_FILE_NAME}, ${BUILD_PLAT}/${FIP_NAME})
	${Q}${ECHO} "Added BL2 and DTB to ${BUILD_PLAT}/${FIP_NAME} successfully"
	${Q}${FIPTOOL} info ${BUILD_PLAT}/${FIP_NAME}
	$(eval ACTUAL_FIP_SIZE = $(shell \
				stat --printf="%s" ${BUILD_PLAT}/${FIP_NAME}))
	@if [ ${ACTUAL_FIP_SIZE} -gt ${FIP_MAXIMUM_SIZE_10} ]; then \
		echo "FIP image exceeds the maximum size of" \
		     "0x${FIP_MAXIMUM_SIZE}"; \
		false; \
	fi

DTB_SIZE		:= 0x2000
BL2_BASE		:= 0x34302000
$(eval $(call add_define,BL2_BASE))
DTB_BASE		:= $(shell echo 0x$$( $(call hexbc, $(BL2_BASE), -, $(DTB_SIZE)) ) )
$(eval $(call add_define,DTB_BASE))

all: call_mkimage
call_mkimage: add_to_fip ${MKIMAGE_FIP_CONF_FILE} ${FIP_HDR_SIZE_FILE} ${BL2_W_DTB_SIZE_FILE}
	${Q}${ECHO} "  MKIMAGE ${BUILD_PLAT}/fip.s32"
	${Q}FIP_HDR_SIZE=$$(cat ${FIP_HDR_SIZE_FILE}); \
	BL2_W_DTB_SIZE=$$(cat ${BL2_W_DTB_SIZE_FILE}); \
	LOAD_ADDRESS=0x$$($(call hexbc, ${BL2_BASE}, -, $${FIP_HDR_SIZE}, -, ${DTB_SIZE})); \
	$(call run_mkimage, ${BL2_BASE}, $${LOAD_ADDRESS}, ${MKIMAGE_FIP_CONF_FILE}, ${BUILD_PLAT}/${FIP_NAME}, ${BUILD_PLAT}/fip.s32)
	${Q}${ECHO} "Generated ${BUILD_PLAT}/fip.s32 successfully"

# If BL32_EXTRA1 option is present, include the binary it is pointing to
# in the FIP image
ifneq ($(BL32_EXTRA1),)
$(eval $(call TOOL_ADD_IMG,bl32_extra1,--tos-fw-extra1))
endif
