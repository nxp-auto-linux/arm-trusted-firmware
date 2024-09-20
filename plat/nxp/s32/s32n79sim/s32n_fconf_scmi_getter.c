/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <s32n_fconf_scmi_getter.h>

/**
 * Do not use a populate function, as the properties
 * are not read from the device tree.
 */
struct scmi_sh_mem_config scmi_sh_mem_config = {0};
struct scmi_fss_channel_config scmi_fss_ch_config = {0};
