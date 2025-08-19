// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2025 NXP
 */

#ifndef S32N_SCMI_EXTENSIONS_H
#define S32N_SCMI_EXTENSIONS_H

#include <stdint.h>

/*
 * Reset Event Types. Should be kept in sync with FSS.
 */
enum plat_reset_event_type {
	RESET_UNDEFINED,
	POWER_ON_RESET,
	DESTRUCTIVE_RESET,
	DESTRUCTIVE_RESET_DURING_STANDBY,
	FUNCTIONAL_RESET,
	FUNCTIONAL_RESET_DURING_STANDBY,
	STARTUP_SELFTEST_COMPLETE,
	WAKEUP_TO_RUN,
	WAKEUP_TO_HK
};

/*
 * Reset Platform Reason
 */
struct scmi_reset_plat_reason_p2a {
	int32_t status;
	uint32_t plat_reset_reason;
} __packed;

/*
 * Identifiers of the Reset Protocol commands extensions
 */
enum scmi_reset_command_extensions_id {
	SCMI_RESET_GET_PLATFORM_REASON = 0x25,
};

#endif  /* S32N_SCMI_EXTENSIONS_H */
