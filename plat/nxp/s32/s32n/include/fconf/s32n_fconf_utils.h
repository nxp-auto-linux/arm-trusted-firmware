/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32N_FCONF_UTILS_H
#define S32N_FCONF_UTILS_H

#include <stdint.h>

/**
 * fconf_get_node_info() - get device tree node information including base address and size
 * @dtb:        Pointer to the device tree blob
 * @offset:     Starting offset for compatible string search (typically -1 to search from root),
 *              or the direct node offset when compatible is NULL
 * @compatible: Compatible string to search for the node. If NULL, the offset parameter
 *              is used directly as the node offset.
 * @base:       Pointer to store the base address extracted from the node's 'reg' property
 * @size:       Pointer to store the size extracted from the node's 'reg' property
 *
 * This function retrieves the base address and size of a device tree node's 'reg' property.
 * The node can be located either by compatible string or by using a provided node offset.
 *
 * Returns:     On success, returns the node offset (>= 0)
 *              On failure, returns a negative error code: -EINVAL if invalid parameters
 *              are provided or a proper FDT error code
 */
int fconf_get_node_info(const void *config, int offset, const char *compatible,
	uintptr_t *base, size_t *size);

#endif /* S32N_FCONF_UTILS_H */
