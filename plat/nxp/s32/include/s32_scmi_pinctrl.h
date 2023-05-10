/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _S32_SCMI_PINCTRL_H_
#define _S32_SCMI_PINCTRL_H_

#include <lib/utils_def.h>

int s32_scmi_pinctrl_set_mux(const uint16_t *pins, const uint16_t *funcs,
			     const unsigned int no);

#endif /* _S32_SCMI_PINCTRL_H_ */

