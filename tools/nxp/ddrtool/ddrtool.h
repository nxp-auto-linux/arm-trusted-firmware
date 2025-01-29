/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DDRTOOL_H
#define DDRTOOL_H

#include <stdint.h>

#define DDR_FW_VERSION 0x1
#define DDR_FW_MAGIC_NUM 0xDDF1C0DE
#define DDR_FW_END 0xDDF11111

/*
 *
 * ------------------------ DDR FW
 * |    Magic Number      |
 * ------------------------
 * |    Version           |
 * ------------------------
 * |    ddrc_size         |
 * ------------------------
 * |    dq_swap_size      |
 * ------------------------
 * |    phy_size          |
 * ------------------------
 * |    imem_1d_size      |
 * ------------------------
 * |    dmem_1d_size      |
 * ------------------------
 * |    imem_2d_size      |
 * ------------------------
 * |    dmem_2d_size      |
 * ------------------------
 * |    pie_size          |
 * ------------------------
 * |    csr_to_store_size |
 * ------------------------
 * |    ddrc_to_store_size|
 * ------------------------
 * |    ddrc_cfg[]        |
 * ------------------------
 * |    dq_swap_cfg[]     |
 * ------------------------
 * |    phy_cfg[]         |
 * ------------------------
 * |    imem_1d_cfg[]     |
 * ------------------------
 * |    dmem_1d_cfg[]     |
 * ------------------------
 * |    imem_2d_cfg[]     |
 * ------------------------
 * |    dmem_2d_cfg[]     |
 * ------------------------
 * |    pie_cfg[]         |
 * ------------------------
 * |    csr_to_store[]    |
 * ------------------------
 * |    ddrc_to_store[]   |
 * ------------------------
 * |    End code          |
 * ------------------------
 *
 */

typedef struct __packed {
	uint32_t magic;
	uint32_t version;
	uint32_t ddrc_size;
	uint32_t dq_swap_size;
	uint32_t phy_size;
	uint32_t imem_1d_size;
	uint32_t dmem_1d_size;
	uint32_t imem_2d_size;
	uint32_t dmem_2d_size;
	uint32_t pie_size;
	uint32_t csr_to_store_size;
	uint32_t ddrc_to_store_size;
} ddr_fw_header_t;

typedef struct {
	ddr_fw_header_t ddr_hdr;
	struct regconf_16 *phy_cfg;
	struct regconf_16 *pie_cfg;
	struct dqconf *dq_swap_cfg;
	struct regconf *ddrc_cfg;
	uint16_t *imem_1d_cfg;
	uint16_t *imem_2d_cfg;
	uint16_t *dmem_1d_cfg;
	uint16_t *dmem_2d_cfg;
	const uint32_t *csr_to_store;
	const uint32_t *ddrc_to_store;
	uint32_t ddr_fw_end;
} ddr_fw_t;

#define S32CC_DDR_FW_INIT() \
((ddr_fw_t) { \
	.ddr_hdr.magic = DDR_FW_MAGIC_NUM, \
	.ddr_hdr.version = DDR_FW_VERSION, \
	.ddr_hdr.ddrc_size = ddrc_cfg_size, \
	.ddr_hdr.dq_swap_size = dq_swap_cfg_size, \
	.ddr_hdr.phy_size = phy_cfg_size, \
	.ddr_hdr.pie_size = pie_cfg_size, \
	.ddr_hdr.imem_1d_size = imem_1d_cfg_size, \
	.ddr_hdr.dmem_1d_size = dmem_1d_cfg_size, \
	.ddr_hdr.imem_2d_size = imem_2d_cfg_size, \
	.ddr_hdr.dmem_2d_size = dmem_2d_cfg_size, \
	.ddr_hdr.csr_to_store_size = csr_to_store_size, \
	.ddr_hdr.ddrc_to_store_size = ddrc_to_store_size, \
	.ddrc_cfg = ddrc_cfg, \
	.dq_swap_cfg = dq_swap_cfg, \
	.phy_cfg = phy_cfg, \
	.imem_1d_cfg = imem_1d_cfg, \
	.dmem_1d_cfg = dmem_1d_cfg, \
	.imem_2d_cfg = imem_2d_cfg, \
	.dmem_2d_cfg = dmem_2d_cfg, \
	.pie_cfg = pie_cfg, \
	.csr_to_store = csr_to_store, \
	.ddrc_to_store = ddrc_to_store, \
	.ddr_fw_end = DDR_FW_END, \
})

typedef struct {
	bool enable_ecc;
	bool enable_perf;
} ddr_params_t;

#endif /* DDRTOOL_H */
