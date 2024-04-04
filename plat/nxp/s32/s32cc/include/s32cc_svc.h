/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef S32CC_SVC_H
#define S32CC_SVC_H

#include <stdbool.h>

#define S32_SCMI_AGENT_PLAT     0
#define S32_SCMI_AGENT_OSPM     1

static inline bool is_plat_agent(unsigned int agent_id)
{
	return agent_id == S32_SCMI_AGENT_PLAT;
}

#endif /* S32CC_SVC_H */
