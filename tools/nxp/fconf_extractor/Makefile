#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

TOP_DIR := ../../../
HOSTDB ?= gdb

MAKE_HELPERS_DIRECTORY := ${TOP_DIR}/make_helpers/
include ${MAKE_HELPERS_DIRECTORY}build_macros.mk
include ${MAKE_HELPERS_DIRECTORY}build-rules.mk
include ${MAKE_HELPERS_DIRECTORY}build_env.mk
include ${MAKE_HELPERS_DIRECTORY}defaults.mk

FCONF_EXTRACTOR_LOG ?= ${BUILD_PLAT}/fconf_extractor_log

LIBFDT_DIR = ${TOP_DIR}/lib/libfdt

LIBFDT_SOURCES := \
	${LIBFDT_DIR}/fdt_addresses.c \
	${LIBFDT_DIR}/fdt.c \
	${LIBFDT_DIR}/fdt_empty_tree.c \
	${LIBFDT_DIR}/fdt_overlay.c \
	${LIBFDT_DIR}/fdt_ro.c \
	${LIBFDT_DIR}/fdt_rw.c \
	${LIBFDT_DIR}/fdt_strerror.c \
	${LIBFDT_DIR}/fdt_sw.c \
	${LIBFDT_DIR}/fdt_wip.c \

FCONF_EXTR_SOURCES := \
	$(addprefix ${TOP_DIR}/,${FCONF_EXTR_PLAT_SOURCES}) \
	${LIBFDT_SOURCES} \
	${TOP_DIR}/common/fdt_wrappers.c \
	${TOP_DIR}/common/uuid.c \
	${TOP_DIR}/lib/libc/strlcpy.c \
	fconf_extractor.c \

FCONF_EXTR_CFLAGS += -Wall -Werror -std=c99 -g3 -O0
FCONF_EXTR_DEFINES += DEBUG _GNU_SOURCE _XOPEN_SOURCE=700

FCONF_EXTR_INCLUDE_DIRS += \
	include \
	${TOP_DIR}/include \
	${TOP_DIR}/include/common \
	${TOP_DIR}/include/lib/libfdt \
	$(addprefix ${TOP_DIR}/,${FCONF_EXTR_PLAT_INCLUDE_DIRS}) \

.PHONY: all clean distclean

all: ${FCONF_GEN_SRC}

$(eval $(call MAKE_TOOL,$(BUILD_PLAT)/tools,fconf_extractor,FCONF_EXTR))

${FCONF_GEN_SRC}: ${DST} | $$(@D)/
	${ECHO} "  CREATE  $@"
	${Q}${HOSTDB} -q -batch \
		-x "gdb_parser.py" \
		-ex "set pagination off" \
		-ex "set confirm off" \
		-ex "set breakpoint pending on" \
		-ex "break breakpoint" \
		-ex "run ${FCONF_DTB_FILE}" \
		-ex "dump-fconf-structures $@ ${FCONF_GDB_VAR_DUMP} --include ${FCONF_GDB_VAR_INCLUDES}" \
		--args "${DST}" > ${FCONF_EXTRACTOR_LOG}

clean:
	$(Q)rm -rf ${DST} ${FCONF_EXTRACTOR_LOG}

distclean: clean
