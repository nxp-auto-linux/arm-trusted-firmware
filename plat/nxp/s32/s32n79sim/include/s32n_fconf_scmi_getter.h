/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_FCONF_SCMI_GETTER_H
#define S32N_FCONF_SCMI_GETTER_H

#include <lib/fconf/fconf.h>

#define s32n__scmi_ospm_ch_config_getter(prop) scmi_ospm_ch_config.prop
#define s32n__scmi_sh_mem_config_getter(prop) scmi_sh_mem_config.prop
#define s32n__scmi_fss_ch_config_getter(prop) scmi_fss_ch_config.prop

struct scmi_ospm_channel_config {
	uint64_t ospm_mem_base;
	size_t ospm_mem_size;
};

struct scmi_sh_mem_config {
	uint64_t sh_mem_base;
	size_t sh_mem_size;
};

struct scmi_fss_channel_config {
	uint64_t mb_base;
	size_t mb_size;
	uint8_t mru_ch;
};

extern struct scmi_ospm_channel_config scmi_ospm_ch_config;
extern struct scmi_sh_mem_config scmi_sh_mem_config;
extern struct scmi_fss_channel_config scmi_fss_ch_config;

#endif /* S32N_FCONF_SCMI_GETTER_H */
