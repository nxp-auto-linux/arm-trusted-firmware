// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2023-2024 NXP
 */

#ifndef SCMI_MSG_NVMEM_H
#define SCMI_MSG_NVMEM_H

#include <stdint.h>

#define SCMI_PROTOCOL_VERSION_NVMEM	0x10000U

/*
 * Identifiers of the SCMI NVMEM Protocol commands
 */
enum scmi_nvmem_command_id {
	SCMI_NVMEM_READ_CELL = 0x3,
	SCMI_NVMEM_WRITE_CELL = 0x4,
};

/*
 * NVMEM Read Cell
 */
struct scmi_nvmem_read_cell_a2p {
	uint32_t offset;
	uint32_t bytes;
};

struct scmi_nvmem_read_cell_p2a {
	int32_t status;
	uint32_t bytes;
	uint32_t value;
};

/*
 * NVMEM Write Cell
 */
struct scmi_nvmem_write_cell_a2p {
	uint32_t offset;
	uint32_t bytes;
	uint32_t value;
};

struct scmi_nvmem_write_cell_p2a {
	int32_t status;
	uint32_t bytes;
};

#endif /* SCMI_MSG_NVMEM_H */
