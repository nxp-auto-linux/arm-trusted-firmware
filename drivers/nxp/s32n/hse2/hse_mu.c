/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * NXP HSE2 Driver - MU interface
 *
 * Copyright 2025 NXP
 */

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <common/debug.h>

#include "hse_mu.h"

#define HSE_STATUS_SHIFT    16u /* HSE global status shift value inside FSR */

#define HSE_CH_MASK_ALL     GENMASK_32(7, 0) /* all channels interrupt mask */

#define HSE_EVT_MASK_ERR    GENMASK_32(7, 0) /* fatal error GSR mask */
#define HSE_EVT_MASK_WARN   GENMASK_32(15, 8) /* warning GSR mask */
#define HSE_EVT_MASK_INTL   GENMASK_32(31, 16) /* NXP internal flag GSR mask */
#define HSE_EVT_MASK_ALL    GENMASK_32(31, 0) /* all events GSR mask */

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
	uint8_t reserved0[4]; /* 0x4 */
	uint32_t sr;
	uint8_t reserved1[240]; /* 0xF0 */
	uint32_t fcr;
	const uint32_t fsr;
	uint8_t reserved2[8]; /* 0x8 */
	uint32_t gier;
	uint32_t gcr;
	uint32_t gsr;
	uint8_t reserved3[4]; /* 0x4 */
	uint32_t tcr;
	const uint32_t tsr;
	uint32_t rcr;
	const uint32_t rsr;
	uint8_t reserved4[208]; /* 0xD0 */
	uint32_t tr[8];
	uint8_t reserved5[96]; /* 0x60 */
	const uint32_t rr[8];
};

/**
 * struct hse_mu_data - MU interface private data
 * @regs: MU instance register space base virtual address
 * @initialized: MU driver initialized/uninitialized flag
 */
static struct hse_mu_data {
	struct hse_mu_regs *regs;
	bool initialized;
} mu;

/**
 * hse_mu_check_status - return the HSE global status
 *
 * Return: 16 most significant bits of MU instance FSR
 */
uint16_t hse_mu_check_status(void)
{
	uint32_t fsrval;

	if (!mu.initialized)
		return 0;

	fsrval = mmio_read_32((uintptr_t)&mu.regs->fsr);
	fsrval = fsrval >> HSE_STATUS_SHIFT;

	return (uint16_t)fsrval;
}

/**
 * hse_mu_irq_disable - disable a specific type of interrupt using a mask
 * @irq_type: interrupt type
 * @irq_mask: interrupt mask
 */
static void hse_mu_irq_disable(enum hse_irq_type irq_type, uint32_t irq_mask)
{
	uint32_t *regaddr;

	switch (irq_type) {
	case HSE_INT_ACK_REQUEST:
		regaddr = &mu.regs->tcr;
		irq_mask &= HSE_CH_MASK_ALL;
		break;
	case HSE_INT_RESPONSE:
		regaddr = &mu.regs->rcr;
		irq_mask &= HSE_CH_MASK_ALL;
		break;
	case HSE_INT_SYS_EVENT:
		regaddr = &mu.regs->gier;
		irq_mask &= HSE_EVT_MASK_ALL;
		break;
	default:
		return;
	}

	mmio_clrbits_32((uintptr_t)regaddr, irq_mask);
}

/**
 * hse_mu_channel_available - check service channel status
 * @channel: channel index
 *
 * The 8 LSB of MU instance FSR are used by HSE for signaling channel status
 * as busy after a service request has been sent, until the HSE reply arrives.
 *
 * Return: true for selected channel available, false otherwise
 */
bool hse_mu_channel_available(uint8_t channel)
{
	uint32_t fsrval, tsrval, rsrval;

	if (!mu.initialized || channel >= HSE_NUM_CHANNELS)
		return false;

	fsrval = mmio_read_32((uintptr_t)&mu.regs->fsr) & BIT_32(channel);
	tsrval = mmio_read_32((uintptr_t)&mu.regs->tsr) & BIT_32(channel);
	rsrval = mmio_read_32((uintptr_t)&mu.regs->rsr) & BIT_32(channel);

	if (fsrval || !tsrval || rsrval)
		return false;

	return true;
}

/**
 * hse_mu_msg_pending - check if a service request response is pending
 * @channel: channel index
 *
 * Return: true for reply pending, false otherwise
 */
bool hse_mu_msg_pending(uint8_t channel)
{
	uint32_t rsrval;

	if (!mu.initialized || channel >= HSE_NUM_CHANNELS)
		return false;

	rsrval = mmio_read_32((uintptr_t)&mu.regs->rsr) & BIT_32(channel);

	return rsrval ? true : false;
}

/**
 * hse_mu_msg_send - send a message over MU (non-blocking)
 * @channel: channel index
 * @msg: input message
 *
 * Return: 0 on success, -ENODEV for driver not initialized, -EINVAL for
 *         invalid parameter, -EBUSY for selected channel busy
 */
int hse_mu_msg_send(uint8_t channel, uint32_t msg)
{
	if (!mu.initialized)
		return -ENODEV;

	if (channel >= HSE_NUM_CHANNELS)
		return -EINVAL;

	if (!hse_mu_channel_available(channel))
		return -EBUSY;

	mmio_write_32((uintptr_t)&mu.regs->tr[channel], msg);

	return 0;
}

/**
 * hse_mu_msg_recv - read a message received over MU (non-blocking)
 * @channel: channel index
 * @msg: output message
 *
 * Return: 0 on success, -ENODEV for driver not initialized, -EINVAL for
 *         invalid parameter, -ENOMSG for no message queued
 */
int hse_mu_msg_recv(uint8_t channel, uint32_t *msg)
{
	if (!mu.initialized)
		return -ENODEV;

	if (channel >= HSE_NUM_CHANNELS || !msg)
		return -EINVAL;

	if (!hse_mu_msg_pending(channel))
		return -ENOMSG;

	*msg = mmio_read_32((uintptr_t)&mu.regs->rr[channel]);

	return 0;
}

/**
 * hse_mu_init - initial setup of MU interface
 * @regs_base: MU registers base address
 *
 * Return: 0 on success, error code otherwise
 */
int hse_mu_init(uintptr_t regs_base)
{
	uint8_t channel;
	uint32_t msg;
	int rc;

	if (mu.initialized)
		return 0;

	mu.regs = (void *)regs_base;

	/* disable all interrupt sources */
	hse_mu_irq_disable(HSE_INT_ACK_REQUEST, HSE_CH_MASK_ALL);
	hse_mu_irq_disable(HSE_INT_RESPONSE, HSE_CH_MASK_ALL);
	hse_mu_irq_disable(HSE_INT_SYS_EVENT, HSE_EVT_MASK_ALL);

	mu.initialized = true;

	/* discard previous pending messages, if any */
	for (channel = 0; channel < HSE_NUM_CHANNELS; channel++) {
		if (!hse_mu_msg_pending(channel))
			continue;

		rc = hse_mu_msg_recv(channel, &msg);
		if (!rc)
			WARN("HSE2: msg %08X dropped from MU channel %d\n",
			     msg, channel);
	}

	return 0;
}
