Copyright 2025 NXP

SCMI NXP Misc Protocol v1.0
===========================

Protocol Overview
-----------------
This protocol is an extension of the `Arm System Control and Management Interface (SCMI)
<http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/index.html>`_.

The goal of this protocol is to provide information with respect to the underlying firmware.

It is added as a Vendor Extension to SCMI, with the ID of 0x86.
The ID is within the range that the SCMI specification provides for platform-specific extensions (0x80 - 0xFF).

For further information on protocol identifiers refer to section 4.1.2 of the SCMI specification.

Protocol Commands
-----------------

PROTOCOL_VERSION
~~~~~~~~~~~~~~~~
On success, this command returns the version of the protocol. For this
version of the specification the return value must be 0x10000, which
corresponds to 1.0.

---------------------------

| message_id: 0x0
| protocol_id: 0x86

This command is mandatory.

---------------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

uint32 version:
   For this version of the specification the return value must be 0x10000.

---------------------------

PROTOCOL_ATTRIBUTES
~~~~~~~~~~~~~~~~~~~
This command returns the implementation details associated with this
protocol.

---------------------------

| message_id: 0x1
| protocol_id: 0x86

This command is mandatory.

---------------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

uint32 attributes:
   | Bits [31:0]: reserved, must be zero.

---------------------------

PROTOCOL_MESSAGE_ATTRIBUTES
~~~~~~~~~~~~~~~~~~~~~~~~~~~
On success, this command returns the implementation details associated
with a specific message in this protocol. In addition to the standard
status codes described in section 4.1.4 of the SCMI specification, the
command can return the error ``NOT_FOUND`` if the message identified by
``message_id`` is not provided by the implementation.

---------------------------

| message_id: 0x2
| protocol_id: 0x86

This command is mandatory.

---------------------------

Parameters:

uint32 message_id:
   message_id of the message.

---------------------------

Return values:

int32 status:
   ``NOT_FOUND`` if the message identified by ``message_id`` is not
   provided by the implementation or one of the status code definitions
   described by section 4.1.4 of the SCMI specification.

uint32 attributes:
   Flags associated with a specific command in the protocol.
   For all commands in this protocol this parameter has a value of 0.

---------------------------

SCMI NXP Misc Messages
----------------------

GET_FIRMWAREINFO
~~~~~~~~~~~~~~~~
On success, this command returns the information from Fss_FW_Info.

---------------------------

| message_id: 0x3
| protocol_id: 0x86

---------------------------

Return values:

int32 status:
	One of the status code definitions described by section 4.1.4 of the
	SCMI specification.

uint32 hotfix_version:
	Immediate revision to fix critical issues.

uint32 version:
	Revision of the firmware in the following format:
	| Bits [31:24]: Major version
	| Bits [23:16]: Minor version
	| Bits [15:8]: Patch version
	| Bits [7:0]: Code drop version

uint32 build_date[3]:
	Firmware build date.

uint32 build_time[3]:
	Firmware build time.

uint32 fimrware_type:
	Firmware type:
	| 0: Standard FW targeting all customers.
	| 1: Premium FW targeting all customers.
	| 2-7: Reserved
	| 8-...: Custom1, Custom2,...

uint32 derivative:
	Firmware derivative:
	| 0: S32N7
	| 1: S32N5
	| 2-...: Reserved for future use.

uint32 hw_support:
    Board support.

uint32 external_device_support:
    External device support.

---------------------------

GET_DATA_VERSION
~~~~~~~~~~~~~~~~
On success, this command returns the versions of the resources.

---------------------------

| message_id: 0x5
| protocol_id: 0x86

---------------------------

Return values:

int32 status:
	One of the status code definitions described by section 4.1.4 of the
	SCMI specification.

uint32 boot_data_version:
	Boot data version.

uint32 clock_data_version:
	Clock data version.

uint32 dsc_irq_data_version:
	DSC IRQ data version.

uint32 eim_data_version:
	EIM data version.

uint32 erm_data_version:
	ERM data version.

uint32 fccu_data_version:
	FCCU data version.

uint32 mem_data_version:
	MEM data version.

uint32 pin_data_version:
	PIN data version.

uint32 pwr_data_version:
	PWR data version.

uint32 rst_data_version:
	RST data version.

uint32 secman_data_version:
	SECMAN data version.

uint32 sns_data_version:
	SNS data version.

uint32 swpu_data_version:
	SWPU data version.
