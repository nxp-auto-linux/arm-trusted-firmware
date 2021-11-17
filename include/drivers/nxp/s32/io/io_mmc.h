/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 * This file is based on 'include/drivers/st/io/io_mmc.h'.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef IO_MMC_H
#define IO_MMC_H

#include <drivers/io/io_driver.h>

#define ROUND_TO_MMC_BLOCK_SIZE(x) \
	(((x) & ~(MMC_BLOCK_MASK)) == (x) ? (x) : \
	 ((x) & ~(MMC_BLOCK_MASK)) + (MMC_BLOCK_SIZE))

int register_io_dev_mmc(const io_dev_connector_t **dev_con);

#endif /* IO_MMC_H */
