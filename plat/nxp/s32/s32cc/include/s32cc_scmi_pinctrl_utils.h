/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32CC__S32_SCMI_PINCTRL_H_
#define S32CC__S32_SCMI_PINCTRL_H_

#include <lib/utils_def.h>

int s32_scmi_pinctrl_set_mux(const uint16_t *pins, const uint16_t *funcs,
			     const unsigned int no);
int s32_scmi_pinctrl_set_pcf(const uint16_t *pins, const unsigned int no_pins,
			     const uint32_t *configs,
			     const unsigned int no_configs);

#endif /* S32CC__S32_SCMI_PINCTRL_H_ */

