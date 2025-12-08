/*
 * Copyright 2022-2025 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/nxp/s32/hse/hse_core.h>
#include <drivers/nxp/s32/hse/hse_mem.h>
#include <drivers/nxp/s32/hse/hse_mu.h>
#include <drivers/nxp/s32/hse/hse_utils.h>
#include <drivers/delay_timer.h>
#include <errno.h>
#include <hse_interface.h>
#include <s32cc_platform_def.h>
#include <stdint.h>
#include <string.h>

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
	bool rmem_slots_mapped[HSE_RMEM_SLOTS_NUM];
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
	hseSrvDescriptor_t srv_desc = {0};
	hseAttrFwVersion_t *buf = NULL;
	int ret;

	buf = hse_mem_alloc(sizeof(*buf));
	if (!buf)
		return -ENOMEM;

	srv_desc.srvId = HSE_SRV_ID_GET_ATTR;
	srv_desc.hseSrv.getAttrReq.attrId = HSE_FW_VERSION_ATTR_ID;
	srv_desc.hseSrv.getAttrReq.attrLen = sizeof(*buf);
	srv_desc.hseSrv.getAttrReq.pAttr = hse_virt_to_phys(buf);

	ret = hse_srv_req_sync(HSE_CHANNEL_ADMIN, &srv_desc);
	if (ret)
		goto out;

	hse_memcpy(&drv.firmware_version, buf, sizeof(*buf));

out:
	hse_mem_free(buf);
	return ret;
}

/**
 * hse_hash_req_set_blen - set hash length for HASH service
 * @hash_req_srv_desc: address of HASH service descriptor
 * @md_size: hash buffer length
 * @hash_len_buf: pointer to buffer to be allocated for keeping hash buffer length
 *
 * Return: 0 on succes, specific errno code on error
 */
int hse_hash_req_set_blen(hseHashSrv_t *hash_req_srv_desc, uint32_t md_size,
			  void **hash_len_buf)
{
	*hash_len_buf = hse_mem_alloc(sizeof(md_size));
	if (!*hash_len_buf)
		return -ENOMEM;

	hse_memcpy(*hash_len_buf, &md_size, sizeof(md_size));

	hash_req_srv_desc->pHashLength = hse_virt_to_phys(*hash_len_buf);

	hse_flush_range((uintptr_t)*hash_len_buf, sizeof(uint32_t));

	return 0;
}

/**
 * hse_hash_req_free - free HASH service resources
 * @hash_len_buf: hash length buffer
 */
void hse_hash_req_free(void *hash_len_buf)
{
	hse_mem_free(hash_len_buf);
}

/**
 * hse_sig_req_set_blen - set signature length for Digital Signature service
 * @sig_req_srv_desc: address of Digital Signature service descriptor
 * @len: signature buffer length
 * @sig_len_buf: pointer to buffer to be allocated for keeping signature buffer length
 * @sig_buf: signature buffer
 *
 * Return: 0 on succes, specific errno code on error
 */
int hse_sig_req_set_blen(hseSignSrv_t *sig_req_srv_desc, size_t len,
			 void **sig_len_buf, void *sig_buf)
{
	*sig_len_buf = hse_mem_alloc(sizeof(len));
	if (!*sig_len_buf)
		return -ENOMEM;

	hse_memcpy(*sig_len_buf, &len, sizeof(len));

	sig_req_srv_desc->pSignatureLength[0] = hse_virt_to_phys(*sig_len_buf);
	sig_req_srv_desc->pSignatureLength[1] = 0u;
	sig_req_srv_desc->pSignature[0] = hse_virt_to_phys(sig_buf);
	sig_req_srv_desc->pSignature[1] = 0u;

	return 0;
}

/**
 * hse_sig_req_free - free Digital Signature service resources
 * @sig_len_buf: signature length buffer
 */
void hse_sig_req_free(void *sig_len_buf)
{
	hse_mem_free(sig_len_buf);
}

/**
 * hse_mac_req_set_tlen - set tag length for MAC service
 * @mac_req_srv_desc: address of MAC service descriptor
 * @mac_len: mac tag length
 * @mac_len_buf: pointer to buffer to be allocated for keeping mac tag length
 *
 * Return: 0 on succes, specific errno code on error
 */
int hse_mac_req_set_tlen(hseMacSrv_t *mac_req_srv_desc, uint32_t mac_len,
			 void **mac_len_buf)
{
	*mac_len_buf = hse_mem_alloc(sizeof(mac_len));
	if (!*mac_len_buf)
		return -ENOMEM;

	hse_memcpy(*mac_len_buf, &mac_len, sizeof(mac_len));

	mac_req_srv_desc->pTagLength = hse_virt_to_phys(*mac_len_buf);

	return 0;
}

/**
 * hse_mac_req_update_tlen - update  tag length for MAC service
 * @mac_len: address of mac tag length
 * @mac_len_buf: buffer containing the updated mac tag length
 */
void hse_mac_req_update_tlen(uint32_t *mac_len, void *mac_len_buf)
{
	hse_memcpy(mac_len, mac_len_buf, sizeof(*mac_len));
}

/**
 * hse_mac_req_free - free MAC service resources
 * @mac_len_buf: mac tag length buffer
 */
void hse_mac_req_free(void *mac_len_buf)
{
	hse_mem_free(mac_len_buf);
}

/**
 * is_secboot_active - checks if the platform has booted in secure mode
 *
 * Return: true if secure boot is active, false otherwise
 */
bool is_secboot_active(void)
{
	uint16_t status;

	hse_mu_init();
	status = hse_mu_check_status();

	if (status & HSE_STATUS_BOOT_OK)
		return true;

	return false;
}

/**
 * hse_addr_in_rmem - check if given addr is in reserved memory
 * @addr: address to be checked
 *
 * Return: true if addr in reserved memory, false otherwise
 */
bool hse_addr_in_rmem(uintptr_t addr)
{
	return (drv.res_mem.paddr <= addr && addr < drv.res_mem.paddr + drv.res_mem.size);
}

int hse_use_rmem(enum hse_rmem_slot slot)
{
	struct hse_memmap mapping;
	bool do_map;
	int err;

	if (slot >= HSE_RMEM_SLOTS_NUM)
		return -EINVAL;

	do_map = !drv.rmem_slots_mapped[slot];

	err = map_hse_res_mem(&mapping, slot, do_map);
	if (err) {
		ERROR("HSE reserved memory remap failed.\n");
		return err;
	}

	drv.res_mem.paddr = mapping.paddr;
	drv.res_mem.size = mapping.size;
	if (do_map) {
		err = hse_mem_setup(drv.res_mem.paddr, drv.res_mem.size);
		if (err)
			return err;
		drv.rmem_slots_mapped[slot] = true;
	}

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
int hse_driver_init(enum hse_rmem_slot slot)
{
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

	err = hse_use_rmem(slot);
	if (err)
		return err;
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
