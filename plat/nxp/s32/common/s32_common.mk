#
# Copyright 2025-2026 NXP
#
# SPDX-License-Identifier: BSD-3-Clause
#

define hexbc
echo "obase=16;ibase=16;$$(echo "$1 $2 $3 $4 $5 $6 $7" | tr 'a-x' 'A-X' | sed 's/0X//g')" | bc
endef

define hexalign
$(call hexbc, "'('$1 + $2 - 1')' / $2 '*' $2")
endef
