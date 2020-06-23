/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2020 NXP
 */
#ifndef CRC8_H
#define CRC8_H

#include <stdlib.h>
#include <stdint.h>

/* Default polynomial: x^8 + x^2 + x^1 + 1 */
#define DEFAULT_POLY 0x7U

/**
 * crc8poly() - Calculate and return CRC-8 of the data
 *
 * @seed: CRC8 start value (seed)
 * @poly: The polynomial to be used
 * @buf: Buffer to checksum
 * @len: Length of buffer in bytes
 * @return CRC8 checksum
 */
uint8_t crc8poly(uint8_t seed, uint8_t poly, const uint8_t *buf, size_t len);
#endif
