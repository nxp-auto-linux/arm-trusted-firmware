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
include plat/nxp/s32/s32_ddr.mk

ERRATA_A53_855873	:= 1
ERRATA_A53_836870	:= 1
ERRATA_A53_1530924	:= 1
ERRATA_SPECULATIVE_AT	:= 1

# Tools
AWK ?= gawk
HEXDUMP ?= xxd
SED ?= sed

S32_PLAT	:= plat/nxp/s32
S32_DRIVERS	:= drivers/nxp/s32

ifneq ($(S32_PLAT_SOC),)
$(eval $(call add_define_val,PLAT_$(S32_PLAT_SOC)))
endif

S32CC_EMU		?= 0
$(eval $(call add_define_val,S32CC_EMU,$(S32CC_EMU)))

S32GEN1_DRAM_INLINE_ECC	?= 1
$(eval $(call add_define_val,S32GEN1_DRAM_INLINE_ECC,$(S32GEN1_DRAM_INLINE_ECC)))

BL2_AT_EL3		:= 1

PLAT_INCLUDES 	+= \
			-Idrivers \
			-Iinclude/common/tbbr \
			-Iinclude/drivers \
			-Iinclude/${S32_DRIVERS} \
			-Iinclude/lib \
			-Iinclude/lib/libc \
			-Iinclude/lib/psci \
			-Iinclude/plat/arm/common \
			-Iinclude/plat/arm/soc/common \
			-Iinclude/plat/common \
			-I${S32_PLAT}/include \

PLAT_BL_COMMON_SOURCES += \
			${GICV3_SOURCES} \
			common/fdt_wrappers.c \
			drivers/nxp/uart/linflexuart.c \
			${S32_PLAT}/s32_bl_common.c \
			${S32_PLAT}/s32_dt.c \
			${S32_PLAT}/s32_lowlevel_common.S \
			${S32_PLAT}/s32_sramc.c \
			${S32_PLAT}/s32_sramc_asm.S \
			${S32_PLAT}/s32_linflexuart.c \
			${S32_PLAT}/s32_linflexuart_crash.S \
			${S32_PLAT}/s32_mc_me.c \
			${S32_PLAT}/s32_ncore.c \
			${S32_PLAT}/s32_pinctrl.c \
			drivers/delay_timer/delay_timer.c \
			drivers/delay_timer/generic_delay_timer.c \
			${S32_DRIVERS}/memory_pool.c \
			${S32_DRIVERS}/clk/early_clocks.c \
			${S32_DRIVERS}/clk/enable_clk.c \
			${S32_DRIVERS}/clk/get_rate.c \
			${S32_DRIVERS}/clk/plat_clk.c \
			${S32_DRIVERS}/clk/s32gen1_clk.c \
			${S32_DRIVERS}/rst/s32gen1_rst.c \
			${S32_DRIVERS}/clk/set_par_rate.c \
			${S32_DRIVERS}/i2c/s32_i2c.c \
			${BOOT_INFO_SRC} \

BL2_SOURCES += \
			${XLAT_TABLES_LIB_SRCS} \
			${DDR_DRV_SRCS} \
			common/desc_image_load.c \
			common/fdt_fixup.c \
			drivers/io/io_fip.c \
			drivers/io/io_storage.c \
			drivers/mmc/mmc.c \
			${S32_DRIVERS}/io/io_mmc.c \
			${S32_DRIVERS}/io/io_memmap.c \
			${S32_DRIVERS}/mmc/s32_mmc.c \
			lib/optee/optee_utils.c \
			${S32_PLAT}/s32_bl2_el3.c \
			${S32_PLAT}/s32_storage.c \
			${S32_PLAT}/s32_lowlevel_bl2.S \

BL31_SOURCES += \
			${XLAT_TABLES_LIB_SRCS} \
			drivers/scmi-msg/base.c \
			drivers/scmi-msg/clock.c \
			drivers/scmi-msg/entry.c \
			drivers/scmi-msg/reset_domain.c \
			${S32_DRIVERS}/clk/clk.c \
			${S32_DRIVERS}/clk/fixed_clk.c \
			${S32_DRIVERS}/clk/s32gen1_scmi_clk.c \
			${S32_DRIVERS}/clk/s32gen1_scmi_ids.c \
			plat/common/plat_gicv3.c \
			plat/common/plat_psci_common.c \
			${S32_PLAT}/include/plat_macros.S \
			${S32_PLAT}/s32_bl31.c \
			${S32_PLAT}/s32_lowlevel_bl31.S \
			${S32_PLAT}/s32_scmi_clk.c \
			${S32_PLAT}/s32_scmi_rst.c \
			${S32_PLAT}/s32_svc.c \
			${S32_PLAT}/s32_psci.c \

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

# This config allows the clock driver to set the nearest frequency for a clock
# if the requested one cannot be set. In both cases, an error will be printed
# with the targeted and the actual frequency.
S32_SET_NEAREST_FREQ	?= 0
$(eval $(call add_define_val,S32_SET_NEAREST_FREQ,$(S32_SET_NEAREST_FREQ)))

# Process HSE_SECBOOT flag
ifneq (${HSE_SECBOOT},)
$(eval $(call add_define,HSE_SECBOOT))
endif

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

ifeq ($(MKIMAGE),)
BL33DIR = $(shell dirname $(BL33))
MKIMAGE = $(BL33DIR)/tools/mkimage
endif
MKIMAGE_CFG ?= ${BL33DIR}/u-boot-s32.cfgout

DUMMY_STAGE := ${BUILD_PLAT}/dummy_fip_stage
DUMMY_FIP := ${BUILD_PLAT}/dummy_fip
FIP_HDR_SIZE_FILE := ${BUILD_PLAT}/fip_hdr_size
BOOT_INFO_SRC := ${BUILD_PLAT}/boot_info.c
FIP_OFFSET_FILE = ${BUILD_PLAT}/fip_offset
IVT_LOCATION_FILE = ${BUILD_PLAT}/ivt_location
FIP_MMC_OFFSET_FILE = ${BUILD_PLAT}/fip_mmc_offset_flag
FIP_QSPI_OFFSET_FILE = ${BUILD_PLAT}/fip_qspi_offset_flag
FIP_MEMORY_OFFSET_FILE = ${BUILD_PLAT}/fip_mem_offset_flag
DUMMY_FIP_S32 = ${BUILD_PLAT}/dummy_fip.s32
MKIMAGE_FIP_CONF_FILE = ${BUILD_PLAT}/fip.cfgout
DTB_SIZE_FILE = ${BUILD_PLAT}/dtb_size
BL2_W_DTB_SIZE_FILE = ${BUILD_PLAT}/bl2_w_dtb_size

define hexbc
echo "obase=16;ibase=16;$$(echo "$1 $2 $3 $4 $5 $6 $7" | tr 'a-x' 'A-X' | sed 's/0X//g')" | bc
endef

define update_fip
${FIPTOOL} update --align ${FIP_ALIGN} --tb-fw $1 --soc-fw-config $2 $3
endef

define update_fip_cert
${FIPTOOL} update --align ${FIP_ALIGN} --tb-fw $1 --soc-fw-config $2 --tb-fw-cert $3 $4
endef

define get_fip_hdr_size
printf "0x%x" $$(${FIPTOOL} info $1 | ${AWK} -F'[=,]' '{print strtonum($$2)}' | sort -n | head -n1)
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
	-e $1 -a $2 -T s32ccimage \
	-n $3 -d $4 $5
endef

define hexfilesize
printf "0x%x" $$(stat -c "%s" $1)
endef

${DUMMY_STAGE}: | ${BUILD_PLAT}
	${ECHO} "  CREATE  $@"
	${Q}echo "dummy_stage" > $@

# Replace all fiptool args with a dummy state except '--align' parameter
${DUMMY_FIP}: fiptool ${DUMMY_STAGE} | ${BUILD_PLAT}
	${ECHO} "  FIP     $@"
	${Q}ARGS=$$(echo "${FIP_ARGS}" | sed "s#\(--[^ a]\+\)\s\+\([^ ]\+\)#\1 ${DUMMY_STAGE}#g"); \
		${FIPTOOL} create $${ARGS} "$@_temp"
ifneq (${HSE_SECBOOT},)
	${Q}$(call update_fip_cert, ${DUMMY_STAGE}, ${DUMMY_STAGE}, ${DUMMY_STAGE}, "$@_temp")
else
	${Q}$(call update_fip, ${DUMMY_STAGE}, ${DUMMY_STAGE}, "$@_temp")
endif
	${Q}mv "$@_temp" $@

${DUMMY_FIP_S32}: ${DUMMY_FIP}
	${ECHO} "  MKIMAGE $@"
	${Q}$(call run_mkimage, ${BL2_BASE}, ${BL2_BASE}, ${MKIMAGE_CFG}, $<, $@) 2> /dev/null

${IVT_LOCATION_FILE}: ${DUMMY_FIP_S32}
	${ECHO} "  MKIMAGE $@"
	${Q}${MKIMAGE} -l $< 2>&1 | grep 'IVT Location' | ${AWK} -F':' '{print $$2}' | xargs > $@

FIP_OFFSET_DELTA ?= 0

define save_fip_off
	FIP_OFFSET=0x$$($(call hexbc, $1, +, ${FIP_OFFSET_DELTA})); \
	echo "$${FIP_OFFSET}" > "$2"
endef

${FIP_OFFSET_FILE}: ${DUMMY_FIP_S32}
	${ECHO} "  MKIMAGE $@"
	${Q}OFF=$$(${MKIMAGE} -l $< 2>&1 | grep Application | ${AWK} '{print $$3}');\
	$(call save_fip_off, $${OFF},$@)

ifndef FIP_HDR_SIZE
${FIP_HDR_SIZE_FILE}: ${DUMMY_FIP} ${FIPTOOL}
	${ECHO} "  CREATE  $@"
	${Q}$(call get_fip_hdr_size, ${DUMMY_FIP}) > $@
else
${FIP_HDR_SIZE_FILE}: FORCE
	${ECHO} "  CREATE  $@"
	${Q}echo "${FIP_HDR_SIZE}" > "$@"

endif

ifeq ($(FIP_MMC_OFFSET)$(FIP_QSPI_OFFSET)$(FIP_MEMORY_OFFSET)$(FIP_EMMC_OFFSET),)
${FIP_MMC_OFFSET_FILE}: ${IVT_LOCATION_FILE} ${FIP_OFFSET_FILE}
	${ECHO} "  CREATE  $@"
	${Q}[ "$$(cat "${IVT_LOCATION_FILE}")" = "QSPI" ] && echo "0" > "$@" || cat "${FIP_OFFSET_FILE}" > "$@"

${FIP_QSPI_OFFSET_FILE}: ${IVT_LOCATION_FILE} ${FIP_OFFSET_FILE}
	${ECHO} "  CREATE  $@"
	${Q}[ "$$(cat "${IVT_LOCATION_FILE}")" = "QSPI" ] && cat "${FIP_OFFSET_FILE}" > "$@" || echo "0" > "$@"

${FIP_MEMORY_OFFSET_FILE}: FORCE | ${BUILD_PLAT}
	${ECHO} "  CREATE  $@"
	${Q}echo "0" > "$@"
else
ifdef FIP_MMC_OFFSET
STORAGE_LOCATIONS = 1
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

FIP_MMC_OFFSET ?= 0
FIP_QSPI_OFFSET ?= 0
FIP_MEMORY_OFFSET ?= 0

${FIP_MMC_OFFSET_FILE}: FORCE | ${BUILD_PLAT}
	${ECHO} "  CREATE  $@"
	${Q}echo "${FIP_MMC_OFFSET}" > "$@"

${FIP_QSPI_OFFSET_FILE}: FORCE | ${BUILD_PLAT}
	${ECHO} "  CREATE  $@"
	${Q}echo "${FIP_QSPI_OFFSET}" > "$@"

${FIP_MEMORY_OFFSET_FILE}: FORCE | ${BUILD_PLAT}
	${ECHO} "  CREATE  $@"
	${Q}echo "${FIP_MEMORY_OFFSET}" > "$@"
endif

${DTB_SIZE_FILE}: dtbs
	${ECHO} "  CREATE  $@"
	$(eval FIP_ALIGN_HEX = $(shell printf "0x%x" ${FIP_ALIGN}))
	$(eval DTB_S = $(shell $(call hexfilesize, ${BUILD_PLAT}/fdts/${DTB_FILE_NAME})))
	$(eval DTB_SIZE = 0x$(shell $(call hexbc, ${DTB_S}, /, ${FIP_ALIGN_HEX}, *, ${FIP_ALIGN_HEX}, +, ${FIP_ALIGN_HEX})))
	${Q}echo "${DTB_SIZE}" > $@

${BL2_W_DTB}: bl2 dtbs ${DTB_SIZE_FILE}
	@cp ${BUILD_PLAT}/fdts/${DTB_FILE_NAME} $@
	@dd if=${BUILD_PLAT}/bl2.bin of=$@ seek=$$(printf "%d" ${DTB_SIZE}) status=none oflag=seek_bytes
ifneq (${HSE_SECBOOT},)
	${Q}PADDINGHEX=$$($(call hexfilesize,${BUILD_PLAT}/bl2.bin)); \
	PADDING=$$(printf "%d" $${PADDINGHEX}); \
	SEEKSIZE=$$(echo "$$(printf '%d' ${DTB_SIZE}) + $${PADDING}" | bc); \
	dd if=/dev/zero of=$@ seek=$$SEEKSIZE bs=1 count=$$PADDING
endif

${BOOT_INFO_SRC}: ${FIP_MMC_OFFSET_FILE} ${FIP_EMMC_OFFSET_FILE} ${FIP_QSPI_OFFSET_FILE} ${FIP_MEMORY_OFFSET_FILE} ${FIP_HDR_SIZE_FILE} ${DTB_SIZE_FILE}
	${ECHO} "  CREATE  $@"
	${Q}echo "const unsigned long fip_mmc_offset = $$(cat ${FIP_MMC_OFFSET_FILE});" > ${BOOT_INFO_SRC}
	${Q}echo "const unsigned long fip_qspi_offset = $$(cat ${FIP_QSPI_OFFSET_FILE});" >> ${BOOT_INFO_SRC}
	${Q}echo "const unsigned long fip_mem_offset = $$(cat ${FIP_MEMORY_OFFSET_FILE});" >> ${BOOT_INFO_SRC}
	${Q}echo "const unsigned int fip_hdr_size = $$(cat ${FIP_HDR_SIZE_FILE});" >> ${BOOT_INFO_SRC}
	${Q}echo "const unsigned int dtb_size = $$(cat ${DTB_SIZE_FILE});" >> ${BOOT_INFO_SRC}

${BL2_W_DTB_SIZE_FILE}: ${BL2_W_DTB}
	${ECHO} "  CREATE  $@"
	${Q}$(call hexfilesize, $<) > $@

${MKIMAGE_FIP_CONF_FILE}: ${BL2_W_DTB_SIZE_FILE} ${FIP_HDR_SIZE_FILE} FORCE
	${ECHO} "  CREATE  $@"
	${Q}cp -f ${MKIMAGE_CFG} $@
ifneq (${HSE_SECBOOT},)
	${Q}ACTUAL_FIP_SIZE=$$($(call hexfilesize,${BUILD_PLAT}/${FIP_NAME})); \
	T_SIZE=$$(printf "0x%x" $${ACTUAL_FIP_SIZE}); \
	echo "DATA_FILE SIZE $$T_SIZE" >> $@
else
	${Q}BL2_W_DTB_SIZE=$$(cat ${BL2_W_DTB_SIZE_FILE}); \
	HDR_SIZE=$$(cat ${FIP_HDR_SIZE_FILE}); \
	T_SIZE=0x$$($(call hexbc, $${BL2_W_DTB_SIZE}, +, $${HDR_SIZE})); \
	echo "DATA_FILE SIZE $$T_SIZE" >> $@
endif

FIP_ALIGN := 16
all: add_to_fip
add_to_fip: fip ${BL2_W_DTB}
	$(eval FIP_MAXIMUM_SIZE_10 = $(shell printf "%d\n" ${FIP_MAXIMUM_SIZE}))
ifneq (${HSE_SECBOOT},)
	@dd if=/dev/urandom of=${BUILD_PLAT}/dummy_cert bs=1 count=256
	${Q}$(call update_fip_cert, ${BUILD_PLAT}/bl2_w_dtb.bin, ${BUILD_PLAT}/fdts/${DTB_FILE_NAME}, ${BUILD_PLAT}/dummy_cert, ${BUILD_PLAT}/${FIP_NAME})
else
	${Q}$(call update_fip, ${BUILD_PLAT}/bl2_w_dtb.bin, ${BUILD_PLAT}/fdts/${DTB_FILE_NAME}, ${BUILD_PLAT}/${FIP_NAME})
endif
	${ECHO} "Added BL2 and DTB to ${BUILD_PLAT}/${FIP_NAME} successfully"
	${Q}${FIPTOOL} info ${BUILD_PLAT}/${FIP_NAME}
	$(eval ACTUAL_FIP_SIZE = $(shell \
				stat --printf="%s" ${BUILD_PLAT}/${FIP_NAME}))
	@if [ ${ACTUAL_FIP_SIZE} -gt ${FIP_MAXIMUM_SIZE_10} ]; then \
		echo "FIP image exceeds the maximum size of" \
		     "0x${FIP_MAXIMUM_SIZE}"; \
		false; \
	fi

ifneq (${HSE_SECBOOT},)
BL2_BASE		:= 0x34080000
else
BL2_BASE		:= 0x34302000
endif
$(eval $(call add_define,BL2_BASE))

all: call_mkimage
call_mkimage: add_to_fip ${MKIMAGE_FIP_CONF_FILE} ${FIP_HDR_SIZE_FILE} ${BL2_W_DTB_SIZE_FILE} ${DTB_SIZE_FILE}
	${ECHO} "  MKIMAGE ${BUILD_PLAT}/fip.s32"
	${Q}FIP_HDR_SIZE=$$(cat ${FIP_HDR_SIZE_FILE}); \
	BL2_W_DTB_SIZE=$$(cat ${BL2_W_DTB_SIZE_FILE}); \
	DTB_SIZE=$$(cat ${DTB_SIZE_FILE}); \
	LOAD_ADDRESS=0x$$($(call hexbc, ${BL2_BASE}, -, $${FIP_HDR_SIZE}, -, $${DTB_SIZE})); \
	$(call run_mkimage, ${BL2_BASE}, $${LOAD_ADDRESS}, ${MKIMAGE_FIP_CONF_FILE}, ${BUILD_PLAT}/${FIP_NAME}, ${BUILD_PLAT}/fip.s32)
	${ECHO} "Generated ${BUILD_PLAT}/fip.s32 successfully"

# If BL32_EXTRA1 option is present, include the binary it is pointing to
# in the FIP image
ifneq ($(BL32_EXTRA1),)
$(eval $(call TOOL_ADD_IMG,bl32_extra1,--tos-fw-extra1))
endif
