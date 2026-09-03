/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>

#include <drivers/arm/css/scmi.h>
#include <drivers/scmi.h>
#include <lib/utils_def.h>
#include <s32_scmi.h>
#include <s32n_scmi_local.h>
#include <s32n_scmi_pinctrl.h>

#define SCMI_PINCTRL_ATTRIBUTES		U(0x3)

/* Selector values of the 'flags' field of a PINCTRL_ATTRIBUTES request */
#define SCMI_PINCTRL_PIN_TYPE		U(0x0)
#define SCMI_PINCTRL_GROUP_TYPE		U(0x1)
#define SCMI_PINCTRL_FUNCTION_TYPE	U(0x2)

/* Size of the short name returned by PINCTRL_ATTRIBUTES */
#define SCMI_PINCTRL_NAME_SIZE		U(16)

/*
 * Name formats used by FSS. They must be kept in sync with it, since the
 * names generated here are indistinguishable from the ones FSS would have
 * returned. FSS pads the resource index to at least three digits, so pin 7
 * is named pin_007, while pin 346 is named pin_346.
 */
#define SCMI_PINCTRL_PIN_NAME_FMT	"pin_%03u"
#define SCMI_PINCTRL_FUNCTION_NAME_FMT	"f_%03u"

#define SCMI_PINCTRL_NR_PINS(_attr_low)		((_attr_low) & UINT16_MAX)
#define SCMI_PINCTRL_NR_GROUPS(_attr_low)	((_attr_low) >> 16)
#define SCMI_PINCTRL_NR_FUNCTIONS(_attr_high)	((_attr_high) & UINT16_MAX)

struct scmi_pinctrl_attributes_a2p {
	uint32_t identifier;
	uint32_t flags;
};

struct scmi_pinctrl_attributes_p2a {
	int32_t status;
	uint32_t attributes;
	char name[SCMI_PINCTRL_NAME_SIZE];
};

/*
 * PROTOCOL_ATTRIBUTES response for the pinctrl protocol. The two attribute
 * words pack the resource counts: attributes_low holds the number of pins in
 * its lower half and the number of groups in its upper half, attributes_high
 * holds the number of functions in its lower half.
 */
struct scmi_pinctrl_protocol_attributes_p2a {
	int32_t status;
	uint32_t attributes_low;
	uint32_t attributes_high;
};

/*
 * Pin and function names reported by FSS are derived from the resource index,
 * so generate them here instead of asking FSS for each one of them, which the
 * agent does while probing. The generated names are identical to the ones FSS
 * would have returned.
 *
 * Groups are not handled locally: a group name comes with the number of pins it
 * contains, which is not part of the cached PROTOCOL_ATTRIBUTES data. The same
 * applies to function names as soon as the platform reports any group, because
 * then a function has to report the number of groups it exposes.
 */
static struct {
	uint32_t nr_pins;
	uint32_t nr_functions;
	/* Set once a PROTOCOL_ATTRIBUTES response allowed the local fast path */
	bool pins_valid;
	bool functions_valid;
} pinctrl_cache;

static bool get_resource_count(uint32_t selector, uint32_t *count,
			       const char **name_format)
{
	switch (selector) {
	case SCMI_PINCTRL_PIN_TYPE:
		if (!pinctrl_cache.pins_valid) {
			return false;
		}

		*count = pinctrl_cache.nr_pins;
		*name_format = SCMI_PINCTRL_PIN_NAME_FMT;
		return true;
	case SCMI_PINCTRL_FUNCTION_TYPE:
		if (!pinctrl_cache.functions_valid) {
			return false;
		}

		*count = pinctrl_cache.nr_functions;
		*name_format = SCMI_PINCTRL_FUNCTION_NAME_FMT;
		return true;
	default:
		return false;
	}
}

static bool pinctrl_attributes_handle_request(struct scmi_shared_mem *mem,
					      size_t mem_size)
{
	const struct scmi_pinctrl_attributes_a2p *request;
	struct scmi_pinctrl_attributes_p2a *response;
	const char *name_format;
	uint32_t resource_count;
	uint32_t identifier;
	uint32_t selector;
	uint32_t length;

	/*
	 * The channel is writable by the non-secure world, so read each field
	 * only once and validate the local copies.
	 */
	length = mem->length;
	if (length != sizeof(mem->msg_header) + sizeof(*request)) {
		return false;
	}

	request = (const void *)mem->msg_payload;
	identifier = request->identifier;
	selector = request->flags;

	if (!get_resource_count(selector, &resource_count, &name_format)) {
		return false;
	}

	if (resource_count == 0U || identifier >= resource_count) {
		return false;
	}

	/* Make sure the response fits in the channel before writing it */
	if (!scmi_channel_fits_payload(mem_size, sizeof(*response))) {
		return false;
	}

	response = (void *)mem->msg_payload;
	response->status = SCMI_SUCCESS;
	/*
	 * No extended name is available, so leave the extended name flag
	 * cleared. This is what keeps the agent from following up with a
	 * PINCTRL_NAME_GET request, which is not handled locally.
	 */
	response->attributes = 0U;
	(void)snprintf(response->name, sizeof(response->name), name_format,
		       identifier);
	mem->length = sizeof(mem->msg_header) + sizeof(*response);

	return true;
}

static void protocol_attributes_snoop_response(const struct scmi_shared_mem *mem,
						size_t mem_size)
{
	const struct scmi_pinctrl_protocol_attributes_p2a *response;
	uint32_t attributes_low, attributes_high;

	if (!scmi_msg_has_payload(mem, sizeof(*response)) ||
	    !scmi_channel_fits_payload(mem_size, sizeof(*response))) {
		return;
	}

	response = (const void *)mem->msg_payload;
	if (response->status != SCMI_SUCCESS) {
		return;
	}

	attributes_low = response->attributes_low;
	attributes_high = response->attributes_high;

	pinctrl_cache.nr_pins = SCMI_PINCTRL_NR_PINS(attributes_low);
	pinctrl_cache.nr_functions = SCMI_PINCTRL_NR_FUNCTIONS(attributes_high);

	pinctrl_cache.pins_valid = true;
	pinctrl_cache.functions_valid =
		(SCMI_PINCTRL_NR_GROUPS(attributes_low) == 0U);
}

/*
 * Table indexed by the SCMI message ID. Only the messages that can be served
 * locally have a populated entry; every other index is left zeroed, which means
 * "no local handling" (both callbacks NULL).
 */
static const struct s32n_scmi_msg_ops s32n_scmi_pinctrl_msgs[] = {
	[SCMI_PROTO_ATTR_MSG] = {
		.snoop_response = protocol_attributes_snoop_response,
	},
	[SCMI_PINCTRL_ATTRIBUTES] = {
		.handle_request = pinctrl_attributes_handle_request,
	},
};

const struct s32n_scmi_msg_table s32n_scmi_pinctrl_msg_table = {
	.ops = s32n_scmi_pinctrl_msgs,
	.nr_ops = ARRAY_SIZE(s32n_scmi_pinctrl_msgs),
};
