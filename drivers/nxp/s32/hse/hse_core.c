/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <common/fdt_wrappers.h>
#include <drivers/nxp/s32/hse/hse_core.h>
#include <drivers/nxp/s32/hse/hse_mu.h>
#include <drivers/nxp/s32/hse/hse_utils.h>
#include <drivers/delay_timer.h>
#include <errno.h>
#include <hse_interface.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <s32cc_platform_def.h>
#include <stdint.h>
#include <string.h>
#include "s32cc_dt.h"

#define HSE_REQ_TIMEOUT_MS	2000u
#define HSE_REQ_TIMEOUT_US	200u

/**
 * struct hse_descriptor - HSE service descriptor
 * @data: memory space dedicated to a service descriptor
 */
struct hse_descriptor {
	uint8_t data[HSE_MAX_DESCR_SIZE];
};

/**
 * struct hse_drvdata - HSE driver private data
 * @srv_desc[n].desc: service descriptor virtual address for channel n
 * @srv_desc[n].paddr: service descriptor physical address for channel n
 * @firmware_version: firmware version
 * @res_mem.paddr: reserved memory start offset
 * @res_mem.size: reserved memory size
 * @initialized: driver is initialized/uninitialized
 */
struct hse_drvdata {
	struct {
		struct hse_descriptor *desc;
		uintptr_t paddr;
	} srv_desc[HSE_CHANNEL_NUM];
	hseAttrFwVersion_t firmware_version;
	struct {
		uintptr_t paddr;
		size_t size;
	} res_mem;
	bool initialized;
};

static struct hse_drvdata drv;

/**
 * hse_config_channels - configure channels and manage descriptor space
 *
 * HSE firmware restricts channel zero to administrative services, all the rest
 * are usable for crypto operations.
 */
static inline void hse_config_channels(void)
{
	uintptr_t desc_base_vaddr = hse_mu_desc_base_vaddr();
	uintptr_t desc_base_paddr = hse_mu_desc_base_paddr();
	unsigned int offset;
	uint8_t ch;

	for (ch = HSE_CHANNEL_ADMIN; ch < HSE_CHANNEL_NUM; ch++) {
		offset = ch * HSE_MAX_DESCR_SIZE;
		drv.srv_desc[ch].desc = (struct hse_descriptor *)(desc_base_vaddr + offset);
		drv.srv_desc[ch].paddr = desc_base_paddr + offset;
	}
}

/**
 * hse_get_fw_version - retrieve firmware version
 *
 * Return: 0 on succes, specific err code on error
 */
static int hse_get_fw_version(void)
{
	uint32_t attr_size = sizeof(hseAttrFwVersion_t);
	hseSrvDescriptor_t srv_desc;
	int err;

	srv_desc.srvId = HSE_SRV_ID_GET_ATTR;
	srv_desc.hseSrv.getAttrReq.attrId = HSE_FW_VERSION_ATTR_ID;
	srv_desc.hseSrv.getAttrReq.attrLen = attr_size;
	srv_desc.hseSrv.getAttrReq.pAttr = drv.res_mem.paddr;

	err = hse_srv_req_sync(HSE_CHANNEL_ADMIN, &srv_desc);
	if (err)
		return err;

	memcpy(&drv.firmware_version, (void *)drv.res_mem.paddr, attr_size);

	return 0;
}

/**
 * hse_srv_req_sync - initiate service request and wait for response
 * @channel: selects channel for the service request
 * @srv_desc: address of service descriptor
 *
 * Return: 0 on succes, specific errno code on error
 */
int hse_srv_req_sync(enum hse_ch_type channel, const hseSrvDescriptor_t *srv_desc)
{
	uint32_t srv_rsp;
	bool is_pending = false;
	int ret;

	if (!drv.initialized)
		return -EACCES;

	if (!srv_desc || channel >= HSE_CHANNEL_NUM)
		return -EINVAL;

	memset(drv.srv_desc[channel].desc, 0, sizeof(*drv.srv_desc[channel].desc));
	memcpy(drv.srv_desc[channel].desc, srv_desc, sizeof(*srv_desc));

	ret = hse_mu_msg_send(channel, drv.srv_desc[channel].paddr);
	if (ret)
		return ret;

	do {
		ret = hse_mu_msg_pending(channel, &is_pending);
		if (ret)
			return ret;

	} while (!is_pending);

	ret = hse_mu_msg_recv(channel, &srv_rsp);
	if (ret)
		return ret;

	if (srv_rsp == HSE_SRV_RSP_OK)
		return 0;

	ERROR("HSE Service Request failed (service response: 0x%x).", srv_rsp);
	return -EINVAL;
}

/**
 * hse_driver_init - initializes HSE driver internal resources
 *
 * Return: 0 on succes, specific err code on error
 */
int hse_driver_init(void)
{
	struct hse_memmap mapping;
	uint16_t status;
	int err;

	if (drv.initialized)
		return 0;

	err = hse_mu_init();
	if (err) {
		ERROR("Could not initialize HSE MU\n");
		return err;
	}

	status = hse_mu_check_status();
	if (!(status & HSE_STATUS_INIT_OK)) {
		ERROR("HSE Firmware not initialised\n");
		return -EACCES;
	}

	hse_config_channels();

	err = map_hse_res_mem(&mapping);
	if (err)
		return err;

	drv.res_mem.paddr = mapping.paddr;
	drv.res_mem.size = mapping.size;

	drv.initialized = true;

	err = hse_get_fw_version();
	if (err) {
		drv.initialized = false;
		return err;
	}

	INFO("%s hse firmware, version %d.%d.%d\n",
	     drv.firmware_version.fwTypeId == 0 ? "Standard" :
	     (drv.firmware_version.fwTypeId == 1 ? "Premium" : "Custom"),
	     drv.firmware_version.majorVersion,
	     drv.firmware_version.minorVersion,
	     drv.firmware_version.patchVersion);

	INFO("HSE is successfully initialized\n");

	return 0;
}
