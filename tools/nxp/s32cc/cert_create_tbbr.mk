#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

$(eval $(call add_define,PLAT_DEF_OID))
$(eval $(call add_define,PDEF_EXTS))
$(eval $(call add_define,PDEF_CERTS))

INC_DIR += -I../../plat/nxp/s32/s32cc/include

PDEF_CERT_TOOL_PATH	:= ../nxp/s32cc
PLAT_INCLUDE +=	-I${PDEF_CERT_TOOL_PATH}/include
PLAT_OBJECTS +=	${PDEF_CERT_TOOL_PATH}/src/pdef_tbb_cert.o \
			${PDEF_CERT_TOOL_PATH}/src/pdef_tbb_ext.o
OBJECTS	+= ${PLAT_OBJECTS}
