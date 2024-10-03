#
# Copyright 2020-2024 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

BL31SRAM_SOURCES = plat/common/aarch64/platform_up_stack.S \
		   plat/nxp/s32/s32cc/s32g/bl31_sram/bl31sram_entrypoint.S \
		   plat/nxp/s32/s32cc/s32g/bl31_sram/bl31sram_main.c \
		   plat/nxp/s32/s32cc/s32g/s32g_clocks.c \
		   plat/nxp/s32/s32cc/s32g/s32g_mc_me.c \
		   plat/nxp/s32/s32cc/s32_scp_scmi.c \
		   plat/nxp/s32/s32cc/s32_scp_utils.c \
		   drivers/arm/css/scmi/scmi_common.c \
		   drivers/arm/css/scmi/scmi_sys_pwr_proto.c \
		   ${COMMON_DDR_DRV}/ddr_lp.c \
		   lib/cpus/aarch64/cortex_a53.S \
		   lib/locks/exclusive/${ARCH}/spinlock.S	\
		   ${LIBC_SRCS} \

BL31SRAM_ARRAY_NAME ?= bl31sram
BL31SRAM_ARRAY_LEN  ?= bl31sram_len

BL31SRAM_XXD_SRC = plat/nxp/s32/s32cc/s32g/bl31_sram/bl31_sram.c
BL31SRAM_CODE_REGS_SRC = plat/nxp/s32/s32cc/s32g/bl31_sram/bl31_sram_code_reg.c

BL31SRAM_SRC_DUMP   := ${BL31SRAM_XXD_SRC} ${BL31SRAM_CODE_REGS_SRC}

BL31SRAM_DEFAULT_LINKER_SCRIPT_SOURCE := plat/nxp/s32/s32cc/s32g/bl31_sram/bl31SRAM.ld.S

$(eval $(call MAKE_BL,bl31sram))
BL31SRAM_MAPFILE    := $(call IMG_MAPFILE,bl31sram)
BL31SRAM_ELF        := $(call IMG_ELF,bl31sram)

define get_map_symbol
$(shell grep "$1\s\+=" $2 | xargs | cut -f1 -d' ')
endef

${BL31SRAM_MAPFILE}: ${BL31SRAM_ELF}

${BL31SRAM_CODE_REGS_SRC}: ${BL31SRAM_MAPFILE}
	${ECHO} "  PARSE   $<"
	$(eval RO_START = $(call get_map_symbol,__RO_START__,$<))
	$(eval RO_END = $(call get_map_symbol,__RO_END__,$<))
	@${ECHO} "unsigned int bl31sram_ro_start = ${RO_START};" > $@
	@${ECHO} "unsigned int bl31sram_ro_end = ${RO_END};" >> $@

${BL31SRAM_XXD_SRC}: ${BIN}
	${ECHO} "  XXD     $<"
	@${HEXDUMP} -g4 -u -i $^ $@
	@${SED} -ie "s#[[:alnum:]_]\+\[\]#${BL31SRAM_ARRAY_NAME}[]#g" $@
	@${SED} -ie  "s#^unsigned int [^=]\+= #unsigned int ${BL31SRAM_ARRAY_LEN} = #g" $@
