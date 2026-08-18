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

#include <s32_svc.h>

/*
 * Some SCMI requests coming from the OSPM agent describe static platform data.
 * Serving them in BL31 avoids the round-trip to FSS, which is expensive when
 * the agent walks every resource at probe time.
 *
 * Each protocol which needs such treatment provides one entry in a table
 * indexed by the SCMI protocol ID:
 *  - handle_request() inspects the request left by the agent in the channel and,
 *    if it can be served locally, overwrites it with a complete response and
 *    returns true. Returning false means "forward this request to FSS".
 *  - snoop_response() looks at a response received from FSS, typically to cache
 *    the information needed later by handle_request().
 *
 * Both callbacks are optional and receive:
 *  - mem:        the OSPM channel; its contents are writable by the non-secure
 *                world, so each field must be read exactly once into a local
 *                variable before being validated and used.
 *  - mem_size:   size of the channel, to be used before writing any response.
 *  - msg_header: the message header already read by the dispatcher.
 */
struct s32n_scmi_local_proto {
	bool (*handle_request)(struct scmi_shared_mem *mem, size_t mem_size,
			       uint32_t msg_header);
	void (*snoop_response)(const struct scmi_shared_mem *mem,
			       size_t mem_size, uint32_t msg_header);
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

#endif /* S32N_SCMI_LOCAL_H */
