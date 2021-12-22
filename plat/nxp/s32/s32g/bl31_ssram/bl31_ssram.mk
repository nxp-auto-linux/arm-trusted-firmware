#
# Copyright 2020-2021 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

BL31SSRAM_SOURCES =  plat/nxp/s32/s32g/bl31_ssram/bl31ssram_stacks.S \
		     plat/nxp/s32/s32g/bl31_ssram/bl31ssram_entrypoint.S \
		     plat/nxp/s32/s32g/bl31_ssram/bl31ssram_main.c \
		     plat/nxp/s32/s32g/bl31_ssram/ddr_clk.c \
		     plat/nxp/s32/s32g/s32g_clocks.c \
		     plat/nxp/s32/s32g/s32g_mc_me.c \
		     ${DDR_DRV_SRCS} \
		     ${LIBC_SRCS}

BL31SSRAM_ARRAY_NAME ?= bl31ssram
BL31SSRAM_ARRAY_LEN  ?= bl31ssram_len

BL31SSRAM_SRC_DUMP   := plat/nxp/s32/s32g/bl31_ssram/bl31_ssram.c
BL31SSRAM_LINKERFILE := plat/nxp/s32/s32g/bl31_ssram/bl31SSRAM.ld.S

$(eval $(call MAKE_BL,31SSRAM))

${BL31SSRAM_SRC_DUMP}: ${BIN}
	${ECHO} "  XXD     $<"
	@${HEXDUMP} -g4 -u -i $^ $@
	@${SED} -ie "s#[[:alnum:]_]\+\[\]#${BL31SSRAM_ARRAY_NAME}[]#g" $@
	@${SED} -ie  "s#^unsigned int [^=]\+= #unsigned int ${BL31SSRAM_ARRAY_LEN} = #g" $@
