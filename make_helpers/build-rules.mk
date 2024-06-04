#
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

ifndef build-rules-mk
        build-rules-mk := $(lastword $(MAKEFILE_LIST))

        escape-shell = '$(subst ','\'',$(1))'

        .SECONDEXPANSION:

        %/:
		$(ECHO) '  MD      '$(call escape-shell,$(abspath $@))
		$(Q)mkdir -p $(call escape-shell,$@)
endif
