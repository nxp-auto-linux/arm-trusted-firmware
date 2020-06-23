// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020 NXP
 */

#include "lib/crc8.h"

#define CRC8_POLY(P) ((0x1000U | ((P) << 4U)) << 3U)

uint8_t crc8poly(uint8_t seed, uint8_t poly, const uint8_t *buf, size_t len)
{
	size_t i, j;
	uint16_t crc = seed;

	for (i = 0U; i < len; i++) {
		crc = (crc ^ buf[i]) << 8U;
		for (j = 0U; j < 8U; j++) {
			if (crc & 0x8000U)
				crc = crc ^ CRC8_POLY(poly);
			crc <<= 1U;
		}

		crc >>= 8U;
	}

	return (uint8_t)crc;
}
