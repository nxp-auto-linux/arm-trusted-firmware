/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <lib/mmio.h>
#include <assert.h>
#include <arm/css/scmi/scmi_private.h>
#include <errno.h>
#include <s32cc_scp_scmi.h>

#include "include/s32cc_scmi_pinctrl_utils.h"
#include "include/s32cc_pinctrl.h"

#define SCMI_PINCTRL_PINMUX_SET (5)
#define SCMI_PINCTRL_PINCONF_SET_OVR (7)

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

struct scmi_pinctrl_set_pcf_pins_a2p {
	uint32_t no_pins;
	uint16_t pins[];
};

struct scmi_pinctrl_set_pcf_conf_a2p {
	uint32_t mask;
	uint32_t bool_configs;
	uint32_t slew_rate;
};

struct scmi_pinctrl_set_pcf_request_p2a {
	int32_t status;
};

static int s32_scmi_pinctrl_set_mux_chunk(const uint16_t *pins,
					  const uint16_t *funcs,
					  const unsigned int no)
{
	struct scmi_pinctrl_set_mux_request_a2p *payload_args;
	struct scmi_pinctrl_set_mux_request_p2a *payload_resp;
	uint8_t buffer[S32_SCP_BUF_SIZE];
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

	ret = send_scmi_to_scp((uintptr_t)mbx_mem, sizeof(buffer), PSCI);
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

/* In case we may have unaligned writes which would cause
 * Alignment Exceptions because we don't have caches enabled
 * at this point.
 */
void s32_write_u32(void *address, uint32_t value)
{
	uint16_t temp;

	temp = value & GENMASK(15, 0);
	*(uint16_t *)address = temp;

	temp = (value >> 16) & GENMASK(15, 0);
	*(uint16_t *)(address + 2) = temp;
}

static int s32_scmi_pinctrl_set_pcf_chunk(const uint16_t *pins,
					  const unsigned int no_pins,
					  const uint32_t *configs,
					  const unsigned int no_configs)
{
	struct scmi_pinctrl_set_pcf_pins_a2p *payload_pins;
	struct scmi_pinctrl_set_pcf_conf_a2p *payload_conf;
	struct scmi_pinctrl_set_pcf_request_p2a *payload_resp;
	uint8_t buffer[S32_SCP_BUF_SIZE];
	unsigned int i, cfg, val;
	unsigned int token = 0;
	mailbox_mem_t *mbx_mem;
	uint32_t mask = 0, bool_configs = 0;
	int ret;

	_Static_assert(sizeof(buffer) >= sizeof(*mbx_mem),
		       "SCMI message buffer is too small!");

	if (no_pins > SCMI_MAX_PINS)
		return -EINVAL;

	memset(buffer, 0, sizeof(buffer));

	mbx_mem = (mailbox_mem_t *)buffer;
	mbx_mem->res_a = 0U;
	mbx_mem->status = 0U;
	mbx_mem->res_b = 0UL;
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->len = 4U;
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_PINCTRL,
					      SCMI_PINCTRL_PINCONF_SET_OVR,
					      token);

	payload_pins =
		(struct scmi_pinctrl_set_pcf_pins_a2p *)mbx_mem->payload;

	payload_pins->no_pins = no_pins;
	mbx_mem->len += sizeof(*payload_pins);
	for (i = 0; i < no_pins; i++)
		payload_pins->pins[i] = pins[i];

	mbx_mem->len += no_pins * sizeof(*payload_pins->pins);
	payload_conf = (void *)(payload_pins->pins + no_pins);

	for (i = 0; i < no_configs; i++) {
		cfg = PCF_GET_CFG(configs[i]);
		val = PCF_GET_VAL(configs[i]);

		mask |= BIT_32(cfg);

		if (cfg == PCF_SLEW_RATE)
			s32_write_u32(&payload_conf->slew_rate, val);
		else if (val)
			bool_configs |= BIT_32(cfg);
	}
	s32_write_u32(&payload_conf->mask, mask);
	s32_write_u32(&payload_conf->bool_configs, bool_configs);

	if (check_u32_overflow(mbx_mem->len, sizeof(*payload_conf)))
		return -EINVAL;

	mbx_mem->len += sizeof(*payload_conf);

	ret = send_scmi_to_scp((uintptr_t)mbx_mem, sizeof(buffer), PSCI);
	if (ret)
		return ret;

	/* The payload contains the response filled by send_scmi_to_scp() */
	payload_resp =
		(struct scmi_pinctrl_set_pcf_request_p2a *)mbx_mem->payload;
	ret = payload_resp->status;
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to configure pins %d\n", ret);
		return ret;
	}

	return 0;
}

int s32_scmi_pinctrl_set_pcf(const uint16_t *pins, const unsigned int no_pins,
			     const uint32_t *configs,
			     const unsigned int no_configs)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < no_pins / SCMI_MAX_PINS; i++) {
		ret = s32_scmi_pinctrl_set_pcf_chunk(pins, no_pins,
						     configs, no_configs);
		if (ret)
			return ret;

		pins += SCMI_MAX_PINS;
		configs += SCMI_MAX_PINS;
	}

	if (no_pins % SCMI_MAX_PINS)
		ret = s32_scmi_pinctrl_set_pcf_chunk(pins, no_pins,
						     configs, no_configs);

	return ret;
}
