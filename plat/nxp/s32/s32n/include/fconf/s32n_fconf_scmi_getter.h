/*
 * Copyright 2024-2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_FCONF_SCMI_GETTER_H
#define S32N_FCONF_SCMI_GETTER_H

#include <lib/fconf/fconf.h>

#define s32n__scmi_ospm_ch_config_getter(prop) scmi_ospm_ch_config.prop
#define s32n__scmi_sh_mem_config_getter(prop) scmi_sh_mem_config.prop
#define s32n__scmi_doorbell_tx_config_getter(prop) scmi_doorbell_tx_config.prop
#define s32n__scmi_doorbell_rx_config_getter(prop) scmi_doorbell_rx_config.prop
#define s32n__scmi_fss_ch_tx_config_getter(prop) scmi_fss_ch_tx_config.prop
#define s32n__scmi_fss_ch_rx_config_getter(prop) scmi_fss_ch_rx_config.prop
#define s32n__scmi_aps_mru_config_getter(prop) scmi_aps_mru_config.prop

typedef enum {
	SCMI_TX = 0,
	SCMI_RX,
	SCMI_DIR_INVAL,
} scmi_dir_t;

struct scmi_ospm_channel_config {
	uint64_t ospm_mem_base;
	size_t ospm_mem_size;
};

struct scmi_sh_mem_config {
	uint64_t remote_handshake_addr;
	uint64_t local_handshake_addr;
	size_t remote_handshake_size;
	size_t local_handshake_size;
};

struct scmi_fss_channel_config {
	uint64_t mb_base;
	size_t mb_size;
};

struct scmi_doorbell_config {
	uint64_t base;
	uint64_t db_addr;
	size_t size;
};

struct scmi_mru_config {
	uint32_t irq;
	uint8_t chan;
	uint8_t mbox;
};

extern struct scmi_ospm_channel_config scmi_ospm_ch_config;
extern struct scmi_sh_mem_config scmi_sh_mem_config;
extern struct scmi_doorbell_config scmi_doorbell_tx_config;
extern struct scmi_doorbell_config scmi_doorbell_rx_config;
extern struct scmi_fss_channel_config scmi_fss_ch_tx_config;
extern struct scmi_fss_channel_config scmi_fss_ch_rx_config;
extern struct scmi_mru_config scmi_aps_mru_config;

int fconf_populate_scmi_config(uintptr_t config);

#endif /* S32N_FCONF_SCMI_GETTER_H */
