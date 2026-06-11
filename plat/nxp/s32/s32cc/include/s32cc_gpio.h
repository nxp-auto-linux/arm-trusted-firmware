/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32CC_GPIO_H
#define S32CC_GPIO_H

/*
 * Register the S32CC SIUL2 GPIO operations with the TF-A gpio framework.
 * Maps the SIUL2 register banks into the EL3 address space if needed.
 *
 * Returns 0 on success, or a negative errno when the SIUL2 mapping
 * could not be established (in which case the framework ops are not
 * registered and gpio_*() calls will fail later).
 *
 * Calling this more than once is harmless (the framework just re-binds
 * the same ops vector).
 */
int s32_gpio_init(void);

#endif /* S32CC_GPIO_H */
