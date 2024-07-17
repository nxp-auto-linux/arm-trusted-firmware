/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/nxp/s32/hse/hse_mu.h>
#include <drivers/nxp/s32/hse/hse_utils.h>
#include <errno.h>
#include <hse_interface.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <mmio.h>
#include <utils_def.h>

#define HSE_CH_MASK_ALL		GENMASK_32(15, 0) /* all channels irq mask */
#define HSE_STATUS_MASK		GENMASK_32(31, 16) /* HSE global status FSR mask */
#define HSE_EVT_MASK_ALL	GENMASK_32(31, 0) /* all events GSR mask */

/**
 * enum hse_irq_type - HSE interrupt type
 * @HSE_INT_ACK_REQUEST: TX Interrupt, triggered when HSE acknowledged the
 *                       service request and released the service channel
 * @HSE_INT_RESPONSE: RX Interrupt, triggered when HSE wrote the response
 * @HSE_INT_SYS_EVENT: General Purpose Interrupt, triggered when HSE sends
 *                     a system event, generally an error notification
 */
enum hse_irq_type {
	HSE_INT_ACK_REQUEST = 0u,
	HSE_INT_RESPONSE = 1u,
	HSE_INT_SYS_EVENT = 2u,
};

/**
 * struct hse_mu_regs - HSE Messaging Unit Registers
 * @ver: Version ID Register, offset 0x0
 * @par: Parameter Register, offset 0x4
 * @cr: Control Register, offset 0x8
 * @sr: Status Register, offset 0xC
 * @fcr: Flag Control Register, offset 0x100
 * @fsr: Flag Status Register, offset 0x104
 * @gier: General Interrupt Enable Register, offset 0x110
 * @gcr: General Control Register, offset 0x114
 * @gsr: General Status Register, offset 0x118
 * @tcr: Transmit Control Register, offset 0x120
 * @tsr: Transmit Status Register, offset 0x124
 * @rcr: Receive Control Register, offset 0x128
 * @rsr: Receive Status Register, offset 0x12C
 * @tr[n]: Transmit Register n, offset 0x200 + 4*n
 * @rr[n]: Receive Register n, offset 0x280 + 4*n
 */
struct hse_mu_regs {
	const uint32_t ver;
	const uint32_t par;
	uint32_t cr;
	uint32_t sr;
	uint8_t reserved0[240]; /* 0xF0 */
	uint32_t fcr;
	const uint32_t fsr;
	uint8_t reserved1[8]; /* 0x8 */
	uint32_t gier;
	uint32_t gcr;
	uint32_t gsr;
	uint8_t reserved2[4]; /* 0x4 */
	uint32_t tcr;
	const uint32_t tsr;
	uint32_t rcr;
	const uint32_t rsr;
	uint8_t reserved3[208]; /* 0xD0 */
	uint32_t tr[16];
	uint8_t reserved4[64]; /* 0x40 */
	const uint32_t rr[16];
};

/**
 * struct hse_mu_data - MU interface private data
 * @regs: MU instance register space base virtual address
 * @desc_base_vaddr: descriptor space base virtual address
 * @desc_base_paddr: descriptor space base physical address
 * @initialized: MU driver is initialized/uninitialized
 */
struct hse_mu_data {
	struct hse_mu_regs *regs;
	uintptr_t desc_base_vaddr;
	uintptr_t desc_base_paddr;
	bool initialized;
};

static struct hse_mu_data mu;


/**
 * hse_mu_check_status - check the HSE global status
 *
 * Return: 16 MSB of MU instance FSR, 0 on error
 */
uint16_t hse_mu_check_status(void)
{
	uint32_t fsrval;

	if (!mu.initialized)
		return 0;

	fsrval = mmio_read_32((uintptr_t)&mu.regs->fsr);
	fsrval = (fsrval & HSE_STATUS_MASK) >> 16u;

	return (uint16_t)fsrval;
}

/**
 * get_irq_regaddr - get the register address along with its mask
 * @irq_type: interrupt type
 * @irq_mask: return location for the corresponding interrupt mask
 *
 * Return: HSE register address corresponding to the interrupt type.
 *         Also, *irq_mask will store the interrupt mask.
 *         On error, return NULL.
 */
static uint32_t *get_irq_regaddr(enum hse_irq_type irq_type,
				 uint32_t *irq_mask)
{
	uint32_t *regaddr;

	switch (irq_type) {
	case HSE_INT_ACK_REQUEST:
		regaddr = &mu.regs->tcr;
		*irq_mask &= HSE_CH_MASK_ALL;
		break;
	case HSE_INT_RESPONSE:
		regaddr = &mu.regs->rcr;
		*irq_mask &= HSE_CH_MASK_ALL;
		break;
	case HSE_INT_SYS_EVENT:
		regaddr = &mu.regs->gier;
		*irq_mask &= HSE_EVT_MASK_ALL;
		break;
	default:
		return NULL;
	}

	return regaddr;
}

/**
 * hse_mu_irq_disable - disable a specific type of interrupt using a mask
 * @irq_type: interrupt type
 * @irq_mask: interrupt mask
 */
static void hse_mu_irq_disable(enum hse_irq_type irq_type,
			       uint32_t irq_mask)
{
	uint32_t *regaddr;

	regaddr = get_irq_regaddr(irq_type, &irq_mask);
	if (!regaddr)
		return;

	mmio_clrbits_32((uintptr_t)regaddr, irq_mask);
}

/**
 * hse_mu_channel_available - check service channel status
 * @channel: channel index
 *
 * The 16 LSB of MU instance FSR are used by HSE for signaling channel status
 * as busy after a service request has been sent, until the HSE reply is initialized.
 *
 * Return: 0 on success (channel is avaialable)
 *         -EACCESS if the MU has not been initialized yet
 *         -EINVAL if arguments are invalid
 *         -EBUSY if the channel is busy
 */
static int hse_mu_channel_available(uint8_t channel)
{
	uint32_t fsrval, tsrval, rsrval;

	if (!mu.initialized)
		return -EACCES;

	if (channel >= HSE_NUM_OF_CHANNELS_PER_MU)
		return -EINVAL;

	fsrval = mmio_read_32((uintptr_t)&mu.regs->fsr) & BIT(channel);
	tsrval = mmio_read_32((uintptr_t)&mu.regs->tsr) & BIT(channel);
	rsrval = mmio_read_32((uintptr_t)&mu.regs->rsr) & BIT(channel);

	if (fsrval || !tsrval || rsrval) {
		ERROR("channel %d busy\n", channel);
		return -EBUSY;
	}

	return 0;
}

/**
 * hse_mu_msg_pending - check if a service request response is pending
 * @channel: channel index
 * @is_pending: msg is pending or not (output) if the call is successful
 *
 * Return: 0 if call is successful,
 *         -EACCES if mu is not initialized,
 *         -EINVAL if arguments are invalid,
 */
int hse_mu_msg_pending(uint8_t channel, bool *is_pending)
{
	uint32_t rsrval;

	if (!mu.initialized)
		return -EACCES;

	if (channel >= HSE_NUM_OF_CHANNELS_PER_MU || !is_pending)
		return -EINVAL;

	rsrval = mmio_read_32((uintptr_t)&mu.regs->rsr) & BIT(channel);
	if (!rsrval)
		*is_pending = false;
	else
		*is_pending = true;

	return 0;
}

/**
 * hse_mu_msg_send - send a message over MU (non-blocking)
 * @channel: channel index
 * @msg: input message
 *
 * Return: 0 on success,
 *         -EACCESS if MU driver is not initialized,
 *         -EINVAL for invalid channel,
 *         -EBUSY for selected channel busy
 */
int hse_mu_msg_send(uint8_t channel, uint32_t msg)
{
	int ret;

	if (!mu.initialized)
		return -EACCES;

	if (channel >= HSE_NUM_OF_CHANNELS_PER_MU)
		return -EINVAL;

	ret = hse_mu_channel_available(channel);
	if (ret)
		return ret;

	mmio_write_32((uintptr_t)&mu.regs->tr[channel], msg);

	return 0;
}

/**
 * hse_mu_msg_recv - read a message received over MU (non-blocking)
 * @channel: channel index
 * @msg: output message
 *
 * Return: 0 on success,
 *         -EACCESS if MU driver is not initialized,
 *         -EINVAL for invalid channel or msg pointer,
 *         -EIO if there's no message to be received
 */
int hse_mu_msg_recv(uint8_t channel, uint32_t *msg)
{
	bool is_pending_msg;
	int ret;

	if (!mu.initialized)
		return -EACCES;

	if (!msg || channel >= HSE_NUM_OF_CHANNELS_PER_MU)
		return -EINVAL;

	ret = hse_mu_msg_pending(channel, &is_pending_msg);
	if (ret)
		return ret;

	if (!is_pending_msg) {
		ERROR("no message pending on channel %d\n", channel);
		return -EIO;
	}

	*msg = mmio_read_32((uintptr_t)&mu.regs->rr[channel]);

	return 0;
}

/**
 * hse_mu_desc_base_vaddr - returns the virtual address of the service descriptor's
 *                          memory space dedicated to the MU
 *
 * Return: start physical address of the MU's service descriptor space
 *         in case of success, 0 on error
 */
uintptr_t hse_mu_desc_base_vaddr(void)
{
	if (!mu.initialized)
		return 0;

	return mu.desc_base_vaddr;
}

/**
 * hse_mu_desc_base_paddr - returns the physical address of the service descriptor's
 *                          memory space dedicated to the MU
 *
 * Return: start virtual address of the MU's service descriptor space
 *         in case of success, 0 on error
 */
uintptr_t hse_mu_desc_base_paddr(void)
{
	if (!mu.initialized)
		return 0;

	return mu.desc_base_paddr;
}

/**
 * hse_mu_init - initial setup of MU interface
 *
 * Return: 0 on success, error code otherwise
 */
int hse_mu_init(void)
{
	struct hse_memmap mapping;
	bool has_pending_msg;
	uint8_t channel;
	uint32_t msg;
	int err, res;

	if (mu.initialized)
		return 0;

	err = map_hse_mu_regs(&mapping);
	if (err)
		return err;

	mu.regs = (struct hse_mu_regs *)mapping.paddr;

	err = map_hse_mu_desc(&mapping);
	if (err)
		return err;

	mu.desc_base_paddr = mapping.paddr;
	mu.desc_base_vaddr = mapping.vaddr;

	hse_mu_irq_disable(HSE_INT_ACK_REQUEST, HSE_CH_MASK_ALL);
	hse_mu_irq_disable(HSE_INT_RESPONSE, HSE_CH_MASK_ALL);
	hse_mu_irq_disable(HSE_INT_SYS_EVENT, HSE_EVT_MASK_ALL);

	mu.initialized = true;

	/* discard any pending messages */
	for (channel = 0; channel < HSE_NUM_OF_CHANNELS_PER_MU; channel++) {
		hse_mu_msg_pending(channel, &has_pending_msg);
		if (!has_pending_msg)
			continue;

		res = hse_mu_msg_recv(channel, &msg);
		if (!res)
			INFO("channel %d: msg %08x dropped\n",
			     channel, msg);
	}

	return 0;
}
