#
# Copyright 2021-2023 NXP
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
ERRATA_S32_051700	:= 1

# Tools
AWK ?= gawk
HEXDUMP ?= xxd
SED ?= sed
DD ?= dd status=none
OPENSSL ?= openssl

S32_PLAT	:= plat/nxp/s32
S32_DRIVERS	:= drivers/nxp/s32

ifneq ($(S32_PLAT_SOC),)
$(eval $(call add_define_val,PLAT_$(S32_PLAT_SOC)))
endif

S32CC_EMU		?= 0
$(eval $(call add_define_val,S32CC_EMU,$(S32CC_EMU)))

S32GEN1_DRAM_INLINE_ECC	?= 1
$(eval $(call add_define_val,S32GEN1_DRAM_INLINE_ECC,$(S32GEN1_DRAM_INLINE_ECC)))

S32CC_USE_SCP		?= 0
$(eval $(call add_define_val,S32CC_USE_SCP,$(S32CC_USE_SCP)))

# Use pinctrl over SCMI
S32CC_USE_SCMI_PINCTRL 	?= 0
$(eval $(call add_define_val,S32CC_USE_SCMI_PINCTRL,$(S32CC_USE_SCMI_PINCTRL)))

# Get the reset cause via SCMI
S32CC_USE_SCMI_NVMEM 	?= 0
$(eval $(call add_define_val,S32CC_USE_SCMI_NVMEM,$(S32CC_USE_SCMI_NVMEM)))

# SCMI GPIO fixup of the U-Boot dtb
S32CC_SCMI_GPIO_FIXUP ?= 0
$(eval $(call add_define_val,S32CC_SCMI_GPIO_FIXUP,$(S32CC_SCMI_GPIO_FIXUP)))

# SCMI NVMEM fixup of the U-Boot dtb
S32CC_SCMI_NVMEM_FIXUP ?= 0
$(eval $(call add_define_val,S32CC_SCMI_NVMEM_FIXUP,$(S32CC_SCMI_NVMEM_FIXUP)))

# Enable SCMI message logging
SCMI_LOGGER ?= 0
$(eval $(call add_define_val,SCMI_LOGGER,$(SCMI_LOGGER)))

BL2_AT_EL3		:= 1

PLAT_INCLUDES 	+= \
			-Idrivers \
			-Iinclude/common/tbbr \
			-Iinclude/drivers \
			-Iinclude/${S32_DRIVERS} \
			-Iinclude/${S32_DRIVERS}/ddr \
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
			${S32_PLAT}/s32_mc_rgm.c \
			${S32_PLAT}/s32_ncore.c \
			${S32_PLAT}/s32_pinctrl.c \
			${S32_PLAT}/s32_scmi_pinctrl.c \
			${S32_PLAT}/s32_pmic.c \
			${S32_PLAT}/core_turn_off.c \
			${S32_PLAT}/s32_irq_mgmt.c \
			${S32_PLAT}/s32_scmi_rst.c \
			drivers/delay_timer/delay_timer.c \
			drivers/delay_timer/generic_delay_timer.c \
			drivers/arm/css/scmi/scmi_logger.c \
			${S32_DRIVERS}/memory_pool.c \
			${S32_DRIVERS}/clk/clk.c \
			${S32_DRIVERS}/clk/early_clocks.c \
			${S32_DRIVERS}/clk/enable_clk.c \
			${S32_DRIVERS}/clk/get_rate.c \
			${S32_DRIVERS}/clk/plat_clk.c \
			${S32_DRIVERS}/clk/s32gen1_clk.c \
			${S32_DRIVERS}/rst/s32gen1_rst.c \
			${S32_DRIVERS}/clk/set_par_rate.c \
			${S32_DRIVERS}/stm/s32_stm.c \
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
			${S32_DRIVERS}/scmi_logger/s32_scmi_logger.c \
			lib/optee/optee_utils.c \
			${S32_PLAT}/s32_bl2_el3.c \
			${S32_PLAT}/s32_storage.c \
			${S32_PLAT}/s32_lowlevel_bl2.S \
			${S32_PLAT}/s32_scp_utils.c \
			${S32_PLAT}/s32_scp_scmi.c \
			drivers/arm/css/scmi/scmi_common.c \

BL31_SOURCES += \
			${XLAT_TABLES_LIB_SRCS} \
			drivers/scmi-msg/base.c \
			drivers/scmi-msg/clock.c \
			drivers/scmi-msg/entry.c \
			drivers/scmi-msg/perf.c \
			drivers/scmi-msg/reset_domain.c \
			${S32_DRIVERS}/clk/fixed_clk.c \
			${S32_DRIVERS}/clk/s32gen1_scmi_clk.c \
			${S32_DRIVERS}/clk/s32gen1_scmi_ids.c \
			${S32_DRIVERS}/perf/s32gen1_scmi_perf.c \
			${S32_DRIVERS}/scmi_logger/s32_scmi_logger.c \
			plat/common/plat_gicv3.c \
			plat/common/plat_psci_common.c \
			${S32_PLAT}/include/plat_macros.S \
			${S32_PLAT}/s32_bl31.c \
			${S32_PLAT}/s32_lowlevel_bl31.S \
			${S32_PLAT}/s32_scmi_clk.c \
			${S32_PLAT}/s32_scmi_perf.c \
			${S32_PLAT}/s32_svc.c \
			${S32_PLAT}/s32_psci.c \
			${S32_PLAT}/s32_scp_scmi.c \
			${S32_PLAT}/s32_scp_utils.c \
			drivers/arm/css/scmi/scmi_common.c \
			drivers/arm/css/scmi/scmi_ap_core_proto.c \
			drivers/arm/css/scmi/scmi_pwr_dmn_proto.c \
			drivers/arm/css/scmi/scmi_sys_pwr_proto.c \

DTC_FLAGS		+= -Wno-unit_address_vs_reg
DTC_CPPFLAGS	+= -I${S32_PLAT}/include \
				   -I${S32_SOC_FAMILY}/include \
				   -I${PLAT_SOC_PATH}/include \
				   -I${S32_BOARD_PATH}/include \
				   -Iinclude/common/tbbr \
				   -Iinclude/plat/common \
				   -Iinclude/lib/libc \
				   -Iinclude/arch/aarch64 \

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

ifeq (${SECBOOT_SUPPORT},1)
ifeq (${RSA_PRIV_FIP},)
$(error RSA_PRIV_FIP is not set)
endif
HSE_SUPPORT		:=1
call_mkimage: sign_image
endif

# Process HSE_SUPPORT flag
ifneq (${HSE_SUPPORT},)
$(eval $(call add_define,HSE_SUPPORT))
$(eval $(call add_define,HSE_MU_INST,4))
else
$(eval $(call add_define,HSE_MU_INST,0))
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
BL2_BIN			:= $(strip $(call IMG_BIN,2))
BL2_MAPFILE		:= ${BUILD_PLAT}/bl2/bl2.map
FIP_BIN			:= ${BUILD_PLAT}/${FIP_NAME}
FIP_S32			:= ${BUILD_PLAT}/fip.s32

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
BL2_PADDING = ${BUILD_PLAT}/bl2_padding
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
ifneq (${HSE_SUPPORT},)
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
FIP_MEMORY_OFFSET := 0

${FIP_MMC_OFFSET_FILE}: ${IVT_LOCATION_FILE} ${FIP_OFFSET_FILE}
	${ECHO} "  CREATE  $@"
	${Q}[ "$$(cat "${IVT_LOCATION_FILE}")" = "QSPI" ] && echo "0" > "$@" || cat "${FIP_OFFSET_FILE}" > "$@"

${FIP_QSPI_OFFSET_FILE}: ${IVT_LOCATION_FILE} ${FIP_OFFSET_FILE}
	${ECHO} "  CREATE  $@"
	${Q}[ "$$(cat "${IVT_LOCATION_FILE}")" = "QSPI" ] && cat "${FIP_OFFSET_FILE}" > "$@" || echo "0" > "$@"
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
endif

${FIP_MEMORY_OFFSET_FILE}: FORCE | ${BUILD_PLAT}
	${ECHO} "  CREATE  $@"
	${Q}echo "${FIP_MEMORY_OFFSET}" > "$@"

define get_bl2_sym_addr
$(shell grep "$1.*=" ${BL2_MAPFILE} | ${AWK} '{print $$1}')
endef

ifeq (${FIP_MEMORY_OFFSET},0)
${BL2_PADDING}: FORCE | ${BUILD_PLAT}
	${ECHO} "  CREATE  $@"
	${Q}cat /dev/null > $@
else
${BL2_PADDING}: bl2 FORCE | ${BUILD_PLAT}
	${ECHO} "  CREATE  $@"
	$(eval BL2_END_ADDR = $(call get_bl2_sym_addr,__BL2_END__))
	$(eval BL2_DATA_END = $(call get_bl2_sym_addr,__DATA_END__))
	$(eval BL2_PADDING_SIZE = 0x$(shell $(call hexbc, ${BL2_END_ADDR}, -, ${BL2_DATA_END})))
	@${DD} if=/dev/zero of=$@ seek=0 bs=$$(printf "%d" ${BL2_PADDING_SIZE}) count=1
endif

${DTB_SIZE_FILE}: dtbs
	${ECHO} "  CREATE  $@"
	$(eval FIP_ALIGN_HEX = $(shell printf "0x%x" ${FIP_ALIGN}))
	$(eval DTB_S = $(shell $(call hexfilesize, ${BUILD_PLAT}/fdts/${DTB_FILE_NAME})))
	$(eval DTB_SIZE = 0x$(shell $(call hexbc, ${DTB_S}, /, ${FIP_ALIGN_HEX}, *, ${FIP_ALIGN_HEX}, +, ${FIP_ALIGN_HEX})))
	${Q}echo "${DTB_SIZE}" > $@

${BL2_W_DTB}: bl2 dtbs ${DTB_SIZE_FILE} ${BL2_PADDING}
	@cp ${BUILD_PLAT}/fdts/${DTB_FILE_NAME} $@
	@${DD} if=${BL2_BIN} of=$@ seek=$$(printf "%d" ${DTB_SIZE}) oflag=seek_bytes
	@${DD} if=${BL2_PADDING} conv=notrunc >> $@

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

# required dependency on add_to_fip, otherwise in parallel builds might result in wrong size
${MKIMAGE_FIP_CONF_FILE}: ${BL2_W_DTB_SIZE_FILE} ${FIP_HDR_SIZE_FILE} add_to_fip FORCE
	${ECHO} "  CREATE  $@"
	${Q}cp -f ${MKIMAGE_CFG} $@
	${Q}BL2_W_DTB_SIZE=$$(cat ${BL2_W_DTB_SIZE_FILE}); \
	HDR_SIZE=$$(cat ${FIP_HDR_SIZE_FILE}); \
	T_SIZE=0x$$($(call hexbc, $${BL2_W_DTB_SIZE}, +, $${HDR_SIZE})); \
	echo "DATA_FILE SIZE $$T_SIZE" >> $@

FIP_ALIGN := 16
all: add_to_fip
add_to_fip: fip ${BL2_W_DTB}
	$(eval FIP_MAXIMUM_SIZE_10 = $(shell printf "%d\n" ${FIP_MAXIMUM_SIZE}))
ifneq (${HSE_SUPPORT},)
	@${DD} if=/dev/urandom of=${BUILD_PLAT}/dummy_cert bs=1 count=256
	${Q}$(call update_fip_cert, ${BL2_W_DTB}, ${BUILD_PLAT}/fdts/${DTB_FILE_NAME}, ${BUILD_PLAT}/dummy_cert, ${BUILD_PLAT}/${FIP_NAME})
else
	${Q}$(call update_fip, ${BL2_W_DTB}, ${BUILD_PLAT}/fdts/${DTB_FILE_NAME}, ${BUILD_PLAT}/${FIP_NAME})
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

ifneq (${HSE_SUPPORT},)
BL2_BASE		?= 0x34083000
else
BL2_BASE		?= 0x34302000
endif
$(eval $(call add_define,BL2_BASE))

all: call_mkimage

call_mkimage: add_to_fip ${MKIMAGE_FIP_CONF_FILE} ${FIP_HDR_SIZE_FILE} ${BL2_W_DTB_SIZE_FILE} ${DTB_SIZE_FILE}
	${ECHO} "  MKIMAGE ${BUILD_PLAT}/fip.s32"
	${Q}FIP_HDR_SIZE=$$(cat ${FIP_HDR_SIZE_FILE}); \
	BL2_W_DTB_SIZE=$$(cat ${BL2_W_DTB_SIZE_FILE}); \
	DTB_SIZE=$$(cat ${DTB_SIZE_FILE}); \
	LOAD_ADDRESS=0x$$($(call hexbc, ${BL2_BASE}, -, $${FIP_HDR_SIZE}, -, $${DTB_SIZE})); \
	$(call run_mkimage, ${BL2_BASE}, $${LOAD_ADDRESS}, ${MKIMAGE_FIP_CONF_FILE}, ${FIP_BIN}, ${FIP_S32})
	${ECHO} "Generated ${FIP_S32} successfully"

fiptool_info: add_to_fip
	@${FIPTOOL} info ${FIP_BIN} > ${BUILD_PLAT}/$@

sign_image: fiptool_info add_to_fip
	@$(eval cert_off=$(shell sed -n  "s/.*Trusted Boot Firmware BL2 certificate: offset=\(0x[0-9A-Fa-f]*\).*/\1/p" ${BUILD_PLAT}/fiptool_info))
	${ECHO} "Certificate offset: ${cert_off}"

	@${DD} if=${FIP_BIN} of=${BUILD_PLAT}/tosign-fip.bin bs=$$(printf "%d" ${cert_off}) count=1 2> /dev/null
	@${OPENSSL} dgst -sha1 -sign ${RSA_PRIV_FIP} -out ${BUILD_PLAT}/fip-signature.bin ${BUILD_PLAT}/tosign-fip.bin

	@${FIPTOOL} update --align 16 --tb-fw-cert ${BUILD_PLAT}/fip-signature.bin ${FIP_BIN}

	${ECHO} "${FIP_BIN} has been signed"

# If BL32_EXTRA1 option is present, include the binary it is pointing to
# in the FIP image
ifneq ($(BL32_EXTRA1),)
$(eval $(call TOOL_ADD_IMG,bl32_extra1,--tos-fw-extra1))
endif
