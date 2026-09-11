/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef S32N_SCMI_LOCAL_H
#define S32N_SCMI_LOCAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <arm/css/scmi/scmi_private.h>
#include <s32_svc.h>

/*
 * Some SCMI requests coming from the OSPM agent describe static platform data.
 * Serving them in BL31 avoids the round-trip to FSS, which is expensive when
 * the agent walks every resource at probe time.
 *
 * A protocol served locally provides, per SCMI message ID, a pair of optional
 * callbacks:
 *  - handle_request() inspects the request left by the agent in the channel and,
 *    if it can be served locally, overwrites it with a complete response and
 *    returns true. Returning false means "forward this request to FSS".
 *  - snoop_response() looks at a response received from FSS, typically to cache
 *    the information needed later by handle_request().
 *
 * Both callbacks receive:
 *  - mem:      the OSPM channel; its contents are writable by the non-secure
 *              world, so each field must be read exactly once into a local
 *              variable before being validated and used.
 *  - mem_size: size of the channel, to be used before writing any response.
 */
struct s32n_scmi_msg_ops {
	bool (*handle_request)(struct scmi_shared_mem *mem, size_t mem_size);
	void (*snoop_response)(const struct scmi_shared_mem *mem,
			       size_t mem_size);
};

/* A protocol's ops array indexed by SCMI message ID, and its entry count. */
struct s32n_scmi_msg_table {
	const struct s32n_scmi_msg_ops *ops;
	size_t nr_ops;
};

/*
 * Return true if the request in the channel has been answered locally
 * and should not be forwarded to FSS.
 */
bool s32n_scmi_local_handle_request(struct scmi_shared_mem *mem,
				    size_t mem_size);

/* Update the local caches based on a successful response received from FSS. */
void s32n_scmi_local_snoop_response(const struct scmi_shared_mem *mem,
				    size_t mem_size);

/* True when the device tree describes both the notification buffer and IRQ. */
bool is_ospm_notif_enabled(void);

/* Copy an SCMI message into the OS notification buffer and pend the doorbell. */
int s32n_scmi_relay_notif_to_ospm(mailbox_mem_t *mb);

#endif /* S32N_SCMI_LOCAL_H */
