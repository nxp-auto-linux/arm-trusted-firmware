#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

$(eval $(call add_define,PLAT_DEF_OID))
$(eval $(call add_define,PDEF_EXTS))
$(eval $(call add_define,PDEF_CERTS))

ifeq ($(HAS_DDR_FW),1)
ifeq ($(CUSTOM_DDR_DRV),)
$(eval $(call add_define,PDEF_KEYS))
else
$(eval $(call add_define,CUSTOM_DDR_DRV))
endif
endif

INC_DIR += -I../../plat/nxp/s32/include

PDEF_CERT_TOOL_PATH	:= ../nxp/s32
PLAT_INCLUDE +=	-I${PDEF_CERT_TOOL_PATH}/include
PLAT_OBJECTS +=	${PDEF_CERT_TOOL_PATH}/src/pdef_tbb_cert.o \
			${PDEF_CERT_TOOL_PATH}/src/pdef_tbb_ext.o

ifeq ($(HAS_DDR_FW),1)
ifeq ($(CUSTOM_DDR_DRV),)
PLAT_OBJECTS += ${PDEF_CERT_TOOL_PATH}/src/pdef_tbb_key.o
endif
endif
OBJECTS	+= ${PLAT_OBJECTS}
