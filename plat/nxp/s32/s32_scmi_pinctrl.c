/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/mmio.h>
#include <assert.h>
#include <arm/css/scmi/scmi_private.h>
#include <errno.h>
#include <s32_scp_scmi.h>

#include "include/s32_scmi_pinctrl.h"

#define SCMI_PINCTRL_PINMUX_SET (5)

#define SCMI_MAX_PINS 24

struct scmi_pinctrl_pin_function {
	uint16_t pin;
	uint16_t function;
};

struct scmi_pinctrl_set_mux_request_a2p {
	uint32_t no_pins;
	struct scmi_pinctrl_pin_function pf[];
};

struct scmi_pinctrl_set_mux_request_p2a {
	int32_t status;
};

static int s32_scmi_pinctrl_set_mux_chunk(const uint16_t *pins,
					  const uint16_t *funcs,
					  const unsigned int no)
{
	struct scmi_pinctrl_set_mux_request_a2p *payload_args;
	struct scmi_pinctrl_set_mux_request_p2a *payload_resp;
	uint8_t buffer[S32_SCP_CH_MEM_SIZE];
	unsigned int token = 0;
	mailbox_mem_t *mbx_mem;
	unsigned int i;
	int ret;

	_Static_assert(sizeof(buffer) >= sizeof(*mbx_mem),
		       "SCMI message buffer is too small!");

	if (no > SCMI_MAX_PINS)
		return -EINVAL;

	memset(buffer, 0, sizeof(buffer));

	mbx_mem = (mailbox_mem_t *)buffer;
	mbx_mem->res_a = 0U;
	mbx_mem->status = 0U;
	mbx_mem->res_b = 0UL;
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->len = 4U + sizeof(*payload_args);
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_PINCTRL,
					      SCMI_PINCTRL_PINMUX_SET,
					      token);

	payload_args =
		(struct scmi_pinctrl_set_mux_request_a2p *)mbx_mem->payload;
	payload_args->no_pins = no;
	for (i = 0; i < no; i++) {
		payload_args->pf[i].pin = pins[i];
		payload_args->pf[i].function = funcs[i];
	}

	mbx_mem->len +=
		no * sizeof(struct scmi_pinctrl_pin_function);

	ret = send_scmi_to_scp((uintptr_t)mbx_mem, sizeof(buffer));
	if (ret)
		return ret;

	/* The payload contains the response filled by send_scmi_to_scp() */
	payload_resp =
		(struct scmi_pinctrl_set_mux_request_p2a *)mbx_mem->payload;
	ret = payload_resp->status;
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to configure pins %d\n", ret);
		return ret;
	}

	return 0;
}

int s32_scmi_pinctrl_set_mux(const uint16_t *pins, const uint16_t *funcs,
			     const unsigned int no)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < no / SCMI_MAX_PINS; i++) {
		ret = s32_scmi_pinctrl_set_mux_chunk(pins, funcs,
						     SCMI_MAX_PINS);
		if (ret)
			return ret;

		pins += SCMI_MAX_PINS;
		funcs += SCMI_MAX_PINS;
	}

	if (no % SCMI_MAX_PINS)
		ret = s32_scmi_pinctrl_set_mux_chunk(pins, funcs,
						     no % SCMI_MAX_PINS);

	return ret;
}
