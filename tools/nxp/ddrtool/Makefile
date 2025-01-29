#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

MAKE_HELPERS_DIRECTORY := ../../../make_helpers/
include ${MAKE_HELPERS_DIRECTORY}build_env.mk
include ${MAKE_HELPERS_DIRECTORY}build_macros.mk

DDRTOOL ?= ddrtool${BIN_EXT}
PROJECT := $(notdir ${DDRTOOL})
V ?= 0

S32_DRIVERS	:= drivers/nxp/s32

PLATFORM	:= s32g2 s32g3 s32r45evb s32r45renan
DDR_TOOL_SRCS += $(foreach p, ${PLATFORM}, \
	${p}/${p}_cfg.c \
)

override CPPFLAGS += -D_GNU_SOURCE -D_XOPEN_SOURCE=700
HOSTCCFLAGS := -Wall -Werror -pedantic -std=c99
ifeq (${DEBUG},1)
  HOSTCCFLAGS += -g -O0 -DDEBUG
else
  HOSTCCFLAGS += -O2
endif

INCLUDE_PATHS += -I../../../include \
				 -I../../../${S32_DRIVERS}/ddr \
				 -I../../../include/${S32_DRIVERS}/ddr \

HOSTCCFLAGS += ${DEFINES}

ifeq (${V},0)
  Q := @
else
  Q :=
endif

OBJECTS := ddrtool.o $(DDR_TOOL_SRCS:.c=.o)

DEPS := $(patsubst %.o,%.d,$(OBJECTS))

.PHONY: all clean distclean

all: ${PROJECT}

%.o: %.c Makefile
	@echo "  HOSTCC  $<"
	${Q}${HOSTCC} -c ${CPPFLAGS} ${HOSTCCFLAGS} ${INCLUDE_PATHS} -MD -MP $< -o $@

${PROJECT}: ${OBJECTS} Makefile
	@echo "  HOSTLD  $@"
	${Q}${HOSTCC} ${OBJECTS} -o $@
	@${ECHO_BLANK_LINE}
	@echo "Built $@ successfully"
	@${ECHO_BLANK_LINE}

-include $(DEPS)

clean:
	@echo "Cleaning up..."
	$(call SHELL_DELETE_ALL, ${PROJECT} ${OBJECTS} $(DEPS))
