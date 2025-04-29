// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022-2024 NXP
 */

#include <s32cc_bl_common.h>
#include <s32cc_clocks.h>
#include <s32cc_mc_rgm.h>
#include <s32cc_pinctrl.h>
#include <s32cc_scp_scmi.h>
#include <s32cc_scp_utils.h>

#include <libc/assert.h>
#include <drivers/arm/css/scmi.h>
#include "ddr_utils.h"
#include <arm/css/scmi/scmi_private.h>
#include <clk/s32gen1_clk_funcs.h>
#include <common/debug.h>
#include <drivers/scmi.h>
#include <scmi-msg/clock.h>
#include <scmi-msg/nvmem.h>
#include <scmi-msg/reset_domain.h>

#include <dt-bindings/clock/s32cc-scmi-clock.h>
#include <dt-bindings/nvmem/s32cc-scmi-nvmem.h>
#include <dt-bindings/reset/s32cc-scmi-reset.h>

static int scp_scmi_reset_set_state(uint32_t domain_id, bool assert)
{
	int ret;
	unsigned int token = 0;
	struct scmi_reset_domain_request_a2p *payload_args;
	struct scmi_reset_domain_request_p2a *payload_resp;
	mailbox_mem_t *mbx_mem;
	uint8_t buffer[S32_SCP_BUF_SIZE];

	mbx_mem = (mailbox_mem_t *)buffer;
	mbx_mem->res_a = 0U;
	mbx_mem->status = 0U;
	mbx_mem->res_b = 0UL;
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->len = 4U + sizeof(*payload_args);
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_RESET_DOMAIN,
					      SCMI_RESET_DOMAIN_REQUEST,
					      token);

	payload_args = (struct scmi_reset_domain_request_a2p *)mbx_mem->payload;
	payload_args->domain_id = domain_id;
	payload_args->reset_state = 0;

	if (assert)
		payload_args->flags = SCMI_RESET_DOMAIN_EXPLICIT;
	else
		payload_args->flags = 0U;

	ret = send_scmi_to_scp((uintptr_t)mbx_mem, sizeof(buffer), PSCI);
	if (ret)
		return ret;

	/* The payload contains the response filled by send_scmi_to_scp() */
	payload_resp = (struct scmi_reset_domain_request_p2a *)mbx_mem->payload;
	ret = payload_resp->status;
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to reset domain %u\n", domain_id);
		return ret;
	}

	return 0;
}

static int scp_scmi_clk_set_config(unsigned int clock_index, bool enable)
{
	int ret;
	unsigned int token = 0;
	struct scmi_clock_config_set_a2p *payload_args;
	struct scmi_clock_config_set_p2a *payload_resp;
	mailbox_mem_t *mbx_mem;
	uint8_t buffer[S32_SCP_BUF_SIZE];

	mbx_mem = (mailbox_mem_t *)buffer;
	mbx_mem->res_a = 0U;
	mbx_mem->status = 0U;
	mbx_mem->res_b = 0UL;
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->len = 4U + sizeof(struct scmi_clock_config_set_a2p);
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_CLOCK,
					      SCMI_CLOCK_CONFIG_SET,
					      token);

	payload_args = (struct scmi_clock_config_set_a2p *)mbx_mem->payload;
	payload_args->clock_id = clock_index;

	if (enable)
		payload_args->attributes = SCMI_CLOCK_CONFIG_SET_ENABLE_MASK;
	else
		payload_args->attributes = 0u;

	ret = send_scmi_to_scp((uintptr_t)mbx_mem, sizeof(buffer), PSCI);
	if (ret)
		return ret;

	/* The payload contains the response filled by send_scmi_to_scp() */
	payload_resp = (struct scmi_clock_config_set_p2a *)mbx_mem->payload;
	ret = payload_resp->status;
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to %s clock %u\n", enable ? "enable" : "disable",
		      clock_index);
		return ret;
	}

	return 0;
}

static int scp_scmi_clk_set_config_enable(unsigned int clock_index)
{
	return scp_scmi_clk_set_config(clock_index, true);
}

static int scp_scmi_clk_set_rate(unsigned int clock_index, unsigned long rate)
{
	int ret;
	unsigned int token = 0;
	struct scmi_clock_rate_set_a2p *payload_args;
	struct scmi_clock_rate_set_p2a *payload_resp;
	mailbox_mem_t *mbx_mem;
	uint8_t buffer[S32_SCP_BUF_SIZE];

	mbx_mem = (mailbox_mem_t *)buffer;
	mbx_mem->res_a = 0U;
	mbx_mem->status = 0U;
	mbx_mem->res_b = 0UL;
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->len = 4U + sizeof(struct scmi_clock_rate_set_a2p);
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_CLOCK,
					      SCMI_CLOCK_RATE_SET,
					      token);

	payload_args = (struct scmi_clock_rate_set_a2p *)mbx_mem->payload;
	payload_args->flags = 0;
	payload_args->clock_id = clock_index;
	payload_args->rate[0] = (uint32_t)(rate & GENMASK(31, 0));
	payload_args->rate[1] = (uint32_t)((uint64_t)rate >> 32);

	ret = send_scmi_to_scp((uintptr_t)mbx_mem, sizeof(buffer), PSCI);
	if (ret)
		return ret;

	/* The payload contains the response filled by send_scmi_to_scp() */
	payload_resp = (struct scmi_clock_rate_set_p2a *)mbx_mem->payload;
	ret = payload_resp->status;
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to set clock %u to rate %lu\n", clock_index,
		      rate);
		return ret;
	}

	return 0;
}

static int scp_scmi_nvmem_read_cell(uint32_t offset, uint32_t bytes,
				    uint32_t *value, uint32_t *read_bytes)
{
	int ret = 0;
	unsigned int token = 0;
	struct scmi_nvmem_read_cell_a2p *payload_args;
	struct scmi_nvmem_read_cell_p2a *payload_resp;
	mailbox_mem_t *mbx_mem;
	uint8_t buffer[S32_SCP_BUF_SIZE];

	mbx_mem = (mailbox_mem_t *)buffer;
	mbx_mem->res_a = 0U;
	mbx_mem->status = 0U;
	mbx_mem->res_b = 0UL;
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->len = 4U + sizeof(struct scmi_nvmem_read_cell_a2p);
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_NVMEM,
					      SCMI_NVMEM_READ_CELL,
					      token);

	payload_args = (struct scmi_nvmem_read_cell_a2p *)mbx_mem->payload;
	payload_args->offset = offset;
	payload_args->bytes = bytes;

	ret = send_scmi_to_scp((uintptr_t)mbx_mem, sizeof(buffer), PSCI);
	if (ret)
		return ret;

	/* The payload contains the response filled by send_scmi_to_scp() */
	payload_resp = (struct scmi_nvmem_read_cell_p2a *)mbx_mem->payload;
	ret = payload_resp->status;
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to read nvmem cell at offset %u\n", offset);
		return ret;
	}

	if (payload_resp->bytes != bytes) {
		ERROR("Unexpected number of bytes read: %u\n",
		      payload_resp->bytes);
		return SCMI_E_DENIED;
	}

	*value = payload_resp->value;
	*read_bytes = payload_resp->bytes;

	return 0;
}

static int scp_scmi_nvmem_write_cell(uint32_t offset, uint32_t bytes,
				     uint32_t value, uint32_t *read_bytes)
{
	int ret = 0;
	unsigned int token = 0;
	struct scmi_nvmem_write_cell_a2p *payload_args;
	struct scmi_nvmem_write_cell_p2a *payload_resp;
	mailbox_mem_t *mbx_mem;
	uint8_t buffer[S32_SCP_BUF_SIZE];

	mbx_mem = (mailbox_mem_t *)buffer;
	mbx_mem->res_a = 0U;
	mbx_mem->status = 0U;
	mbx_mem->res_b = 0UL;
	mbx_mem->flags = SCMI_FLAG_RESP_POLL;
	mbx_mem->len = 4U + sizeof(struct scmi_nvmem_write_cell_a2p);
	mbx_mem->msg_header = SCMI_MSG_CREATE(SCMI_PROTOCOL_ID_NVMEM,
					      SCMI_NVMEM_WRITE_CELL,
					      token);

	payload_args = (struct scmi_nvmem_write_cell_a2p *)mbx_mem->payload;
	payload_args->offset = offset;
	payload_args->bytes = bytes;
	payload_args->value = value;

	ret = send_scmi_to_scp((uintptr_t)mbx_mem, sizeof(buffer), PSCI);
	if (ret)
		return ret;

	/* The payload contains the response filled by send_scmi_to_scp() */
	payload_resp = (struct scmi_nvmem_write_cell_p2a *)mbx_mem->payload;
	ret = payload_resp->status;
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to write nvmem cell at offset %u\n", offset);
		return ret;
	}

	if (payload_resp->bytes != bytes) {
		ERROR("Unexpected number of bytes to write: %u\n",
		      payload_resp->bytes);
		return SCMI_E_DENIED;
	}

	*read_bytes = payload_resp->bytes;

	return 0;
}

int scp_a53_clock_setup(void)
{
	int ret;
	uint32_t freq, read_bytes;

	ret = scp_scmi_nvmem_read_cell(S32CC_SCMI_NVMEM_CORE_MAX_FREQ,
				       S32CC_SCMI_NVMEM_CELL_SIZE,
				       &freq, &read_bytes);
	if (ret)
		return ret;

	if (check_u32_mul_overflow(freq, MHZ))
		return -EINVAL;

	return scp_scmi_clk_set_rate(S32CC_SCMI_CLK_A53, freq * MHZ);
}

static int scp_enable_lin_clock(void)
{
	int ret;

	ret = scp_scmi_clk_set_config_enable(S32CC_SCMI_CLK_LINFLEX_XBAR);
	if (ret)
		return ret;

	return scp_scmi_clk_set_config_enable(S32CC_SCMI_CLK_LINFLEX_LIN);
}

static int scp_enable_sdhc_clock(void)
{
	int ret;

	ret = scp_scmi_clk_set_config_enable(S32CC_SCMI_CLK_USDHC_CORE);
	if (ret)
		return ret;

	ret = scp_scmi_clk_set_config_enable(S32CC_SCMI_CLK_USDHC_AHB);
	if (ret)
		return ret;

	ret = scp_scmi_clk_set_config_enable(S32CC_SCMI_CLK_USDHC_MODULE);
	if (ret)
		return ret;

	return scp_scmi_clk_set_config_enable(S32CC_SCMI_CLK_USDHC_MOD32K);
}

static int scp_enable_qspi_clock(void)
{
	int ret;

	ret = scp_scmi_clk_set_config_enable(S32CC_SCMI_CLK_QUADSPI_FLASH1X);
	if (ret)
		return ret;

	ret = scp_scmi_clk_set_config_enable(S32CC_SCMI_CLK_QUADSPI_FLASH2X);
	if (ret)
		return ret;

	ret = scp_scmi_clk_set_config_enable(S32CC_SCMI_CLK_QUADSPI_REG);
	if (ret)
		return ret;

	return scp_scmi_clk_set_config_enable(S32CC_SCMI_CLK_QUADSPI_AHB);
}

static int scp_set_ddr_clock_state(bool enable)
{
	int ret;

	ret = scp_scmi_clk_set_config(S32CC_SCMI_CLK_DDR_PLL_REF, enable);
	if (ret)
		return ret;

	ret = scp_scmi_clk_set_config(S32CC_SCMI_CLK_DDR_AXI, enable);
	if (ret)
		return ret;

	return scp_scmi_clk_set_config(S32CC_SCMI_CLK_DDR_REG, enable);
}

static int scp_enable_ddr_clock(void)
{
	return scp_set_ddr_clock_state(true);
}

static int scp_disable_ddr_clock(void)
{
	return scp_set_ddr_clock_state(false);
}

int scp_periph_clock_init(void)
{
	int ret;

	ret = scp_enable_lin_clock();
	if (ret)
		return ret;

	if (fip_location_mmc) {
		ret = scp_enable_sdhc_clock();
		if (ret)
			return ret;
	} else if (fip_location_qspi) {
		ret = scp_enable_qspi_clock();
		if (ret)
			return ret;
	}

	return scp_enable_ddr_clock();
}

int scp_reset_ddr_periph(void)
{
	int ret;

	ret = scp_disable_ddr_clock();
	if (ret)
		return ret;

	ret = scp_scmi_reset_set_state(S32CC_SCMI_RST_DDR, false);
	if (ret)
		return ret;

	return scp_enable_ddr_clock();
}

int scp_disable_ddr_periph(void)
{
	int ret;

	ret = scp_disable_ddr_clock();
	if (ret)
		return ret;

	return scp_scmi_reset_set_state(S32CC_SCMI_RST_DDR, true);
}

int scp_get_clear_reset_cause(enum reset_cause *cause)
{
	int ret;
	uint32_t value, read_bytes;

	ret = scp_scmi_nvmem_read_cell(S32CC_SCMI_NVMEM_RESET_CAUSE,
				       S32CC_SCMI_NVMEM_CELL_SIZE, &value,
				       &read_bytes);
	if (ret)
		return ret;

	if (value >= CAUSE_MAX_NUM) {
		*cause = CAUSE_MAX_NUM;
		return -EINVAL;
	}

	*cause = (enum reset_cause)value;

	return 0;
}

int scp_is_lockstep_enabled(bool *lockstep_en)
{
	int ret;
	uint32_t value, read_bytes;

	ret = scp_scmi_nvmem_read_cell(S32CC_SCMI_NVMEM_LOCKSTEP_ENABLED,
				       S32CC_SCMI_NVMEM_CELL_SIZE, &value,
				       &read_bytes);
	if (ret)
		return ret;

	*lockstep_en = !!(value);

	return 0;
}

int scp_ddrss_gpr_to_io_retention_mode(void)
{
	int ret;
	uint32_t read_bytes;

	ret = scp_scmi_nvmem_write_cell(S32CC_SCMI_NVMEM_DDR_RET_CTRL,
					S32CC_SCMI_NVMEM_CELL_SIZE,
					0, &read_bytes);
	if (ret)
		return ret;

	return scp_scmi_nvmem_write_cell(S32CC_SCMI_NVMEM_DDR_MEM_RET,
					 S32CC_SCMI_NVMEM_CELL_SIZE,
					 1, &read_bytes);
}
