// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2022-2023 NXP
 */

#include <s32_bl_common.h>
#include <s32_clocks.h>
#include <s32_pinctrl.h>
#include <s32_scp_scmi.h>

#include <libc/assert.h>
#include <drivers/arm/css/scmi.h>
#include <drivers/nxp/s32/ddr/ddr_utils.h>
#include <arm/css/scmi/scmi_private.h>
#include <clk/s32gen1_clk_funcs.h>
#include <common/debug.h>
#include <drivers/scmi.h>
#include <scmi-msg/clock.h>
#include <scmi-msg/reset_domain.h>

#include <dt-bindings/clock/s32gen1-scmi-clock.h>
#include <dt-bindings/reset/s32gen1-scmi-reset.h>

#pragma weak scp_enable_board_early_clocks

int scp_enable_board_early_clocks(void)
{
	return 0;
}

static int scp_scmi_reset_set_state(uint32_t domain_id, bool assert)
{
	int ret;
	unsigned int token = 0;
	struct scmi_reset_domain_request_a2p *payload_args;
	struct scmi_reset_domain_request_p2a *payload_resp;
	mailbox_mem_t *mbx_mem;
	uint8_t buffer[S32_SCP_CH_MEM_SIZE];

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

	ret = send_scmi_to_scp((uintptr_t)mbx_mem, sizeof(buffer));
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

int scp_scmi_clk_set_config_enable(unsigned int clock_index)
{
	int ret;
	unsigned int token = 0;
	struct scmi_clock_config_set_a2p *payload_args;
	struct scmi_clock_config_set_p2a *payload_resp;
	mailbox_mem_t *mbx_mem;
	uint8_t buffer[S32_SCP_CH_MEM_SIZE];

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
	payload_args->attributes = SCMI_CLOCK_CONFIG_SET_ENABLE_MASK;

	ret = send_scmi_to_scp((uintptr_t)mbx_mem, sizeof(buffer));
	if (ret)
		return ret;

	/* The payload contains the response filled by send_scmi_to_scp() */
	payload_resp = (struct scmi_clock_config_set_p2a *)mbx_mem->payload;
	ret = payload_resp->status;
	if (ret != SCMI_E_SUCCESS) {
		ERROR("Failed to enable clock %u\n", clock_index);
		return ret;
	}

	return 0;
}

static int scp_scmi_clk_set_rate(unsigned int clock_index, unsigned long rate)
{
	int ret;
	unsigned int token = 0;
	struct scmi_clock_rate_set_a2p *payload_args;
	struct scmi_clock_rate_set_p2a *payload_resp;
	mailbox_mem_t *mbx_mem;
	uint8_t buffer[S32_SCP_CH_MEM_SIZE];

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

	ret = send_scmi_to_scp((uintptr_t)mbx_mem, sizeof(buffer));
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

int scp_enable_a53_clock(void)
{
	struct siul2_freq_mapping early_freqs;
	int ret;

	ret = s32gen1_get_early_clks_freqs(&early_freqs);
	if (ret)
		return ret;

	return scp_scmi_clk_set_rate(S32GEN1_SCMI_CLK_A53,
				    early_freqs.a53_freq);
}

int scp_enable_lin_clock(void)
{
	int ret;

	ret = scp_scmi_clk_set_config_enable(S32GEN1_SCMI_CLK_LINFLEX_XBAR);
	if (ret)
		return ret;

	return scp_scmi_clk_set_config_enable(S32GEN1_SCMI_CLK_LINFLEX_LIN);
}

int scp_enable_sdhc_clock(void)
{
	int ret;

	ret = scp_scmi_clk_set_config_enable(S32GEN1_SCMI_CLK_USDHC_CORE);
	if (ret)
		return ret;

	ret = scp_scmi_clk_set_config_enable(S32GEN1_SCMI_CLK_USDHC_AHB);
	if (ret)
		return ret;

	ret = scp_scmi_clk_set_config_enable(S32GEN1_SCMI_CLK_USDHC_MODULE);
	if (ret)
		return ret;

	return scp_scmi_clk_set_config_enable(S32GEN1_SCMI_CLK_USDHC_MOD32K);
}

int scp_enable_qspi_clock(void)
{
	int ret;

	ret = scp_scmi_clk_set_config_enable(S32GEN1_SCMI_CLK_QSPI_FLASH1X);
	if (ret)
		return ret;

	ret = scp_scmi_clk_set_config_enable(S32GEN1_SCMI_CLK_QSPI_FLASH2X);
	if (ret)
		return ret;

	ret = scp_scmi_clk_set_config_enable(S32GEN1_SCMI_CLK_QSPI_REG);
	if (ret)
		return ret;

	return scp_scmi_clk_set_config_enable(S32GEN1_SCMI_CLK_QSPI_AHB);
}

int scp_enable_ddr_clock(void)
{
	int ret;

	ret = scp_scmi_clk_set_config_enable(S32GEN1_SCMI_CLK_DDR_PLL_REF);
	if (ret)
		return ret;

	ret = scp_scmi_clk_set_config_enable(S32GEN1_SCMI_CLK_DDR_AXI);
	if (ret)
		return ret;

	return scp_scmi_clk_set_config_enable(S32GEN1_SCMI_CLK_DDR_REG);
}

int s32_scp_plat_clock_init(bool skip_ddr_clk)
{
	int ret;

	/* Request enable clocks via SCMI from SCP */
	ret = scp_enable_a53_clock();
	if (ret)
		return ret;

	ret = scp_enable_lin_clock();
	if (ret)
		return ret;

	if (fip_mmc_offset) {
		ret = scp_enable_sdhc_clock();
		if (ret)
			return ret;
	} else if (fip_qspi_offset) {
		ret = scp_enable_qspi_clock();
		if (ret)
			return ret;
	}

	if (!skip_ddr_clk)
		return scp_enable_ddr_clock();

	return scp_enable_board_early_clocks();
}

int scp_reset_ddr_periph(void)
{
	int ret;

	ret = scp_scmi_reset_set_state(S32GEN1_SCMI_RST_DDR, true);
	if (ret)
		return ret;

	return scp_scmi_reset_set_state(S32GEN1_SCMI_RST_DDR, false);
}
