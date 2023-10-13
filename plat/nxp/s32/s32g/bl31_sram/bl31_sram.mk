#
# Copyright 2020-2023 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

BL31SRAM_SOURCES = plat/common/aarch64/platform_up_stack.S \
		   plat/nxp/s32/s32g/bl31_sram/bl31sram_entrypoint.S \
		   plat/nxp/s32/s32g/bl31_sram/bl31sram_main.c \
		   plat/nxp/s32/s32g/s32g_clocks.c \
		   plat/nxp/s32/s32g/s32g_mc_me.c \
		   plat/nxp/s32/s32_scp_scmi.c \
		   plat/nxp/s32/s32_scp_utils.c \
		   drivers/arm/css/scmi/scmi_common.c \
		   drivers/arm/css/scmi/scmi_sys_pwr_proto.c \
		   ${COMMON_DDR_DRV}/ddr_lp.c \
		   lib/cpus/aarch64/cortex_a53.S \
		   lib/locks/exclusive/${ARCH}/spinlock.S	\
		   ${LIBC_SRCS} \

BL31SRAM_ARRAY_NAME ?= bl31sram
BL31SRAM_ARRAY_LEN  ?= bl31sram_len

BL31SRAM_SRC_DUMP   := plat/nxp/s32/s32g/bl31_sram/bl31_sram.c
BL31SRAM_LINKERFILE := plat/nxp/s32/s32g/bl31_sram/bl31SRAM.ld.S

$(eval $(call MAKE_BL,31SRAM))

${BL31SRAM_SRC_DUMP}: ${BIN}
	${ECHO} "  XXD     $<"
	@${HEXDUMP} -g4 -u -i $^ $@
	@${SED} -ie "s#[[:alnum:]_]\+\[\]#${BL31SRAM_ARRAY_NAME}[]#g" $@
	@${SED} -ie  "s#^unsigned int [^=]\+= #unsigned int ${BL31SRAM_ARRAY_LEN} = #g" $@
