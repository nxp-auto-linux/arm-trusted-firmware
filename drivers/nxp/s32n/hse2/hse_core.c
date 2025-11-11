/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * NXP HSE2 Driver - core
 *
 * Copyright 2025 NXP
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <s32_bl_common.h>
#include <drivers/delay_timer.h>
#include <fconf/s32n_fconf_hse_getter.h>
#include <common/debug.h>

#include <drivers/nxp/s32n/s32n_hse_drv.h>
#include "hse_mu.h"
#include "hse_rmem.h"

#define HSE_REQ_TIMEOUT    2000000u /* request timeout, in microseconds */

#define HSE_ADDRESS_SPACE_LIMIT    0xDFFFFFFFul /* HSE address range limit */

/**
 * struct hse_drvdata - HSE driver private data
 * @mu_base: MU registers base address
 * @rmem_base: reserved memory base address
 * @rmem_size: reserved memory size
 * @srv_desc[n].ptr: service descriptor virtual address for channel n
 * @srv_desc[n].dma: descriptor physical / DMA address for channel n
 * @fw_version: firmware version
 * @initialized: driver initialized/uninitialized flag
 */
static struct hse_drvdata {
	uintptr_t mu_base;
	uintptr_t rmem_base;
	size_t rmem_size;
	struct {
		uint8_t *ptr;
		uintptr_t dma;
	} srv_desc[HSE_NUM_CHANNELS];
	hseAttrFwVersion_t fw_version;
	bool initialized;
} drv;

/**
 * hse_config_channels - configure channels and manage descriptor space
 *
 * Return: 0 on success, -ENOMEM for memory allocation failed
 */
static int hse_config_channels(void)
{
	void *desc_base;
	uint8_t ch;

	/* allocate descriptor space from rmem */
	desc_base = hse_rmem_alloc(HSE_NUM_CHANNELS * HSE_SRV_DESC_MAX_SIZE);
	if (!desc_base)
		return -ENOMEM;

	for (ch = HSE_CHANNEL_ADMIN; ch < HSE_NUM_CHANNELS; ch++) {
		drv.srv_desc[ch].ptr = desc_base + ch * HSE_SRV_DESC_MAX_SIZE;
		drv.srv_desc[ch].dma = (uintptr_t)drv.srv_desc[ch].ptr;
	}

	return 0;
}

/**
 * hse_get_fw_version - retrieve firmware version
 *
 * Return: 0 on success, error code otherwise
 */
static int hse_get_fw_version(void)
{
	hseSrvDescriptor_t srv_desc = {0};
	hseAttrFwVersion_t *buf = NULL;
	int rc;

	buf = hse_mem_alloc(sizeof(*buf));
	if (!buf)
		return -ENOMEM;

	srv_desc.srvId = HSE_SRV_ID_GET_ATTR;
	srv_desc.hseSrv.getAttrReq.attrId = HSE_FW_VERSION_ATTR_ID;
	srv_desc.hseSrv.getAttrReq.attrLen = sizeof(*buf);
	srv_desc.hseSrv.getAttrReq.pAttr = hse_virt_to_phys(buf);

	rc = hse_srv_req_sync(HSE_CHANNEL_ADMIN, &srv_desc);
	if (rc)
		goto out;

	hse_memcpy(&drv.fw_version, buf, sizeof(*buf));

out:
	hse_mem_free(buf);
	return rc;
}

/**
 * hse_get_fw_type_hrf - convert firmware type to human-readable format
 *
 * Return: firmware type in human-readable format
 */
static inline char *hse_get_fw_type_hrf(void)
{
	switch (drv.fw_version.fwTypeId) {
	case 0:
		return "standard";
	case 1:
		return "premium";
	default:
		return "custom";
	}
}

/**
 * hse_print_fw_version - print firmware version
 *
 * Return: 0 on success, error code otherwise
 */
static int hse_print_fw_version(void)
{
	int rc;

	rc = hse_get_fw_version();
	if (rc)
		return rc;

	INFO("HSE2: %s firmware, version %d.%d.%d\n", hse_get_fw_type_hrf(),
	     drv.fw_version.majorVersion, drv.fw_version.minorVersion,
	     drv.fw_version.patchVersion);

	return 0;
}

/**
 * hse_srv_req_sync - send service request to firmware (blocking)
 * @channel: selected channel index
 * @srv_desc: service descriptor virtual address
 *
 * Copy descriptor to rmem and initiate service request, then wait for the
 * HSE reply or until HSE_REQ_TIMEOUT has elapsed. When HSE_CHANNEL_ANY is
 * used, find the first free channel available for crypto services and use it.
 *
 * Return: 0 on success, -ENODEV for driver not initialized, -EINVAL for
 *         invalid parameter, -EBUSY for selected channel busy, -ETIMEDOUT for
 *         request timeout, -ENOMSG for no service response pending, -EBADMSG
 *         for request executed and returned firmware error code reply
 */
int hse_srv_req_sync(uint8_t channel, const hseSrvDescriptor_t *srv_desc)
{
	uint32_t srv_rsp;
	uint64_t timeout;
	int rc;

	if (!drv.initialized)
		return -ENODEV;

	/* find first available channel */
	if (channel == HSE_CHANNEL_ANY)
		for (channel = 1; channel < HSE_NUM_CHANNELS; channel++)
			if (hse_mu_channel_available(channel))
				break;

	if (channel >= HSE_NUM_CHANNELS || !srv_desc)
		return -EINVAL;

	/* copy service descriptor */
	hse_rmem_set(drv.srv_desc[channel].ptr, 0, HSE_SRV_DESC_MAX_SIZE);
	hse_rmem_copy(drv.srv_desc[channel].ptr, srv_desc, sizeof(*srv_desc));

	/* send request */
	rc = hse_mu_msg_send(channel, drv.srv_desc[channel].dma);
	if (rc)
		return rc;

	/* wait for reply */
	timeout = timeout_init_us(HSE_REQ_TIMEOUT);
	while (!hse_mu_msg_pending(channel))
		if (timeout_elapsed(timeout)) {
			ERROR("HSE2: srv request id %08X timed out, channel %d\n",
			      srv_desc->srvId, channel);
			return -ETIMEDOUT;
		}

	/* read reply */
	rc = hse_mu_msg_recv(channel, &srv_rsp);
	if (rc) {
		ERROR("HSE2: srv request id %08X received no reply, channel %d\n",
		      srv_desc->srvId, channel);
		return rc;
	}

	/* check reply */
	if (srv_rsp != HSE_SRV_RSP_OK) {
		ERROR("HSE2: srv request id %08X failed with response %08X, channel %d\n",
		      srv_desc->srvId, srv_rsp, channel);
		return -EBADMSG;
	}

	return 0;
}

static bool hse_addr_in_rmem(uintptr_t addr, size_t size)
{
	if (addr + size < drv.rmem_base)
		return false;

	if (addr > drv.rmem_base + drv.rmem_size)
		return false;

	return true;
}

void *hse_mem_alloc(size_t size)
{
	if (!drv.initialized || !size)
		return NULL;

	return hse_rmem_alloc(size);
}

void hse_mem_free(void *addr)
{
	if (!drv.initialized || !addr || !hse_addr_in_rmem((uintptr_t)addr, 0))
		return;

	return hse_rmem_free(addr);
}

void *hse_memcpy(void *dst, const void *src, size_t size)
{
	if (!drv.initialized || !dst || !src || src == dst || !size)
		return NULL;

	return hse_rmem_copy(dst, src, size);
}

void *hse_memset(void *dst, uint8_t fill, size_t size)
{
	if (!drv.initialized || !dst || !size)
		return NULL;

	return hse_rmem_set(dst, fill, size);
}

/**
 * hse_check_accessible_addr - check if memory location can be accessed by HSE
 * @addr: physical address
 *
 * Return: true for address accessible by HSE, false otherwise
 */
bool hse_check_accessible_addr(uintptr_t addr)
{
	if (addr > HSE_ADDRESS_SPACE_LIMIT)
		return false;

	return true;
}

/**
 * hse_virt_to_phys - physical address translation from virtual address
 * @addr: virtual address in reserved memory range
 *
 * Return: physical address in reserved memory range
 */
uintptr_t hse_virt_to_phys(void *addr)
{
	if (!drv.initialized || !addr || !hse_addr_in_rmem((uintptr_t)addr, 0))
		return 0;

	return (uintptr_t)addr;
}

void hse_flush_range(uintptr_t addr, size_t size)
{
	if (!drv.initialized || !addr || !size || hse_addr_in_rmem(addr, size))
		return;

	flush_dcache_range(addr, size);
}

void hse_inval_range(uintptr_t addr, size_t size)
{
	if (!drv.initialized || !addr || !size || hse_addr_in_rmem(addr, size))
		return;

	inv_dcache_range(addr, size);
}

/**
 * hse_driver_init - initialize HSE driver
 *
 * Return: 0 on success, error code otherwise
 */
int hse_driver_init(void)
{
	uintptr_t base;
	size_t size;
	uint16_t status;
	int rc;

	if (drv.initialized) {
		WARN("HSE2: driver already initialized\n");
		return 0;
	}

	/* map MU registers */
	base = FCONF_GET_PROPERTY(hw_config, hse_config, mu_base);
	size = FCONF_GET_PROPERTY(hw_config, hse_config, mu_size);
	if (!base || !size) {
		ERROR("HSE2: no MU node found enabled in device tree\n");
		return -ENODEV;
	}

	rc = mmap_add_dynamic_region(base, base, MMU_ROUND_UP_TO_PAGE(size),
				     MT_DEVICE | MT_RW | MT_SECURE);
	if (rc < 0) {
		ERROR("HSE2: MU DTB mapping error %d\n", rc);
		return rc;
	}
	drv.mu_base = base;

	/* setup MU interface */
	rc = hse_mu_init(drv.mu_base);
	if (rc) {
		ERROR("HSE2: MU initialization failed\n");
		return rc;
	}

	/* check fw status */
	status = hse_mu_check_status();
	if (!(status & HSE_STATUS_INIT_OK)) {
		ERROR("HSE2: firmware not initialised\n");
		return -ENODEV;
	}

	/* map reserved memory range */
	base = FCONF_GET_PROPERTY(hw_config, hse_config, rmem_base);
	size = FCONF_GET_PROPERTY(hw_config, hse_config, rmem_size);
	if (!base || !size) {
		ERROR("HSE2: no rmem node found enabled in device tree\n");
		return -ENODEV;
	}

	rc = mmap_add_dynamic_region(base, base, MMU_ROUND_UP_TO_PAGE(size),
				     MT_NON_CACHEABLE | MT_RW | MT_SECURE);
	if (rc < 0) {
		ERROR("HSE2: rmem DTB mapping error %d\n", rc);
		return rc;
	}
	drv.rmem_base = base;
	drv.rmem_size = size;

	/* initialize reserved memory range */
	rc = hse_rmem_setup(drv.rmem_base, drv.rmem_size);
	if (rc) {
		ERROR("HSE2: MU initialization failed\n");
		return rc;
	}

	/* configure descriptor space */
	rc = hse_config_channels();
	if (rc) {
		ERROR("HSE2: failed to allocate descriptor space\n");
		return rc;
	}

	drv.initialized = true;

	INFO("HSE2: driver initialized, status 0x%04X\n", status);

	/* check firmware version */
	rc = hse_print_fw_version();
	if (rc)
		WARN("HSE2: failed to retrieve firmware version\n");

	return 0;
}
