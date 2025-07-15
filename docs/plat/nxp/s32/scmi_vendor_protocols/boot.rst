Copyright 2025 NXP

SCMI NXP Boot Protocol v1.0
===========================

Protocol Overview
-----------------
This protocol is an extension of the `Arm System Control and Management Interface (SCMI)
<http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/index.html>`_.

The goal of this protocol is to provide an interface between cohorts and several actions from the boot domain, e.g: Start, Stop, Shutdown, Suspend, wake-up, Get boot info.

It is added as a Vendor Extension to SCMI, with the ID of 0x81.
The ID is within the range that the SCMI specification provides for platform-specific extensions (0x80 - 0xFF).

For further information on protocol identifiers refer to section 4.1.2 of the SCMI specification.

Protocol Commands
-----------------

PROTOCOL_VERSION
~~~~~~~~~~~~~~~~
On success, this command returns the version of the protocol. For this
version of the specification the return value must be 0x1, which
corresponds to 1.0.

---------------------------

| message_id: 0x0
| protocol_id: 0x81

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
| protocol_id: 0x81

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
| protocol_id: 0x81

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

SCMI NXP Boot Messages
----------------------

LOAD_COHORT
~~~~~~~~~~~
On success, this command loads images for a specific cohort.

---------------------------

| message_id: 0x3
| protocol_id: 0x81

---------------------------

Parameters:

uint32 cohort_id:
	Target Cohort ID which receives the requested service.

---------------------------

Return values:

int32 status:
	One of, but not limited to, the following:
	- ``SUCCESS`` in case the target cohort has been successfully loaded.
	- ``DENIED`` in case the requested cohort does not have permission to load the target cohort.
	- ``NOT_FOUND`` in case the target cohort ID does not exist.

	See section 4.1.4 of the SCMI specification for status code definitions.

---------------------------

START_COHORT
~~~~~~~~~~~~
On success, this command starts the cohort manager of a specified cohort.

---------------------------

| message_id: 0x4
| protocol_id: 0x81

---------------------------

Parameters:

uint32 cohort_id:
	Target Cohort ID which receives the requested service.

---------------------------

Return values:

int32 status:
	One of, but not limited to, the following:
	- ``SUCCESS`` in case the target cohort has been successfully started.
	- ``DENIED`` in case the requested cohort does not have permission to start the target cohort.
	- ``NOT_FOUND`` in case the target cohort ID does not exist.

	See section 4.1.4 of the SCMI specification for status code definitions.

---------------------------

BOOT_COHORT
~~~~~~~~~~~
On success, this command boots a cohort by loading the associated images and starting the cohort manager.

---------------------------

| message_id: 0x5
| protocol_id: 0x81

---------------------------

Parameters:

uint32 cohort_id:
	Target Cohort ID which receives the requested service.

---------------------------

Return values:

int32 status:
	One of, but not limited to, the following:
	- ``SUCCESS`` in case the target cohort has been successfully booted.
	- ``DENIED`` in case the requested cohort does not have permission to boot the target cohort.
	- ``NOT_FOUND`` in case the target cohort ID does not exist.

	See section 4.1.4 of the SCMI specification for status code definitions.

---------------------------

REBOOT_COHORT
~~~~~~~~~~~~~
On success, this command reboots a cohort by stopping the cohort, loading the associated images and starting the cohort.

---------------------------

| message_id: 0x6
| protocol_id: 0x81

---------------------------

Parameters:

uint32 cohort_id:
	Target Cohort ID which receives the requested service.

---------------------------

Return values:

int32 status:
	One of, but not limited to, the following:
	- ``SUCCESS`` in case the target cohort has been successfully restarted
	- ``DENIED`` in case the requested cohort does not have permission to restart the target cohort.
	- ``NOT_FOUND`` in case the target cohort ID does not exist.

	See section 4.1.4 of the SCMI specification for status code definitions.

---------------------------

START_COHORT
~~~~~~~~~~~~
On success, this command stops the cohort manager of a specified cohort.

---------------------------

| message_id: 0x7
| protocol_id: 0x81

---------------------------

Parameters:

uint32 cohort_id:
	Target Cohort ID which receives the requested service.

---------------------------

Return values:

int32 status:
	One of, but not limited to, the following:
	- ``SUCCESS`` in case the target cohort has been successfully stopped.
	- ``DENIED`` in case the requested cohort does not have permission to stop the target cohort.
	- ``NOT_FOUND`` in case the target cohort ID does not exist.

	See section 4.1.4 of the SCMI specification for status code definitions.

---------------------------

SHTUDOWN_COHORT
~~~~~~~~~~~~~~~
On success, this command shuts down a specified cohort, by powering down all the cohort resources.

---------------------------

| message_id: 0x8
| protocol_id: 0x81

---------------------------

Parameters:

uint32 cohort_id:
	Target Cohort ID which receives the requested service.

---------------------------

Return values:

int32 status:
	One of, but not limited to, the following:
	- ``SUCCESS`` in case the target cohort has been successfully shutted down.
	- ``DENIED`` in case the requested cohort does not have permission to shut down the target cohort.
	- ``NOT_FOUND`` in case the target cohort ID does not exist.

	See section 4.1.4 of the SCMI specification for status code definitions.

---------------------------

SUSPEND_COHORT
~~~~~~~~~~~~~~
On success, this command suspends a specified cohort, by stopping the execution of a cohort.
The only resources that remain powered are: Wake-up sources and memories configured for retention.

---------------------------

| message_id: 0x9
| protocol_id: 0x81

---------------------------

Parameters:

uint32 cohort_id:
	Target Cohort ID which receives the requested service.
uint32 retention_flag:
	Flag for memory retention.

---------------------------

Return values:

int32 status:
	One of, but not limited to, the following:
	- ``SUCCESS`` in case the target cohort has been successfully suspended.
	- ``DENIED`` in case the requested cohort does not have permission to suspend the target cohort.
	- ``NOT_FOUND`` in case the target cohort ID does not exist.

	See section 4.1.4 of the SCMI specification for status code definitions.

---------------------------

WAKEUP_COHORT
~~~~~~~~~~~~~
On success, this command wakes up a specified cohort that was previously suspended.

---------------------------

| message_id: 0x10
| protocol_id: 0x81

---------------------------

Parameters:

uint32 cohort_id:
	Target Cohort ID which receives the requested service.

---------------------------

Return values:

int32 status:
	One of, but not limited to, the following:
	- ``SUCCESS`` in case the target cohort has been successfully waked up.
	- ``DENIED`` in case the requested cohort does not have permission to wake up the target cohort.
	- ``NOT_FOUND`` in case the target cohort ID does not exist.

	See section 4.1.4 of the SCMI specification for status code definitions.

---------------------------

WARM_RESET_COHORT
~~~~~~~~~~~~~~~~~
On success, this command performs a warm reset of the cohort.
The resources owned by the cohort are reset and only the cohort manager is started.

---------------------------

| message_id: 0x11
| protocol_id: 0x81

---------------------------

Parameters:

uint32 cohort_id:
	Target Cohort ID which receives the requested service.

---------------------------

Return values:

int32 status:
	One of, but not limited to, the following:
	- ``SUCCESS`` in case the target cohort has been successfully restarted.
	- ``DENIED`` in case the requested cohort does not have permission to restart the target cohort.
	- ``NOT_FOUND`` in case the target cohort ID does not exist.

	See section 4.1.4 of the SCMI specification for status code definitions.

---------------------------

GET_BOOT_INFO
~~~~~~~~~~~~~
This command allows the calling agent to request the boot information synchronously.

---------------------------

| message_id: 0x12
| protocol_id: 0x81

---------------------------

Parameters:

uint32 cohort_id:
	Target Cohort ID which receives the requested service.

---------------------------

Return values:

int32 status:
	One of, but not limited to, the following:
	- ``SUCCESS`` in case the boot information of the target cohort has been returned correctly.
	- ``DENIED`` in case the requested cohort does not have permission to get the boot information for the target cohort.
	- ``NOT_FOUND`` in case the target cohort ID does not exist.

	See section 4.1.4 of the SCMI specification for status code definitions.

uint32 config_status:
	Each bit in the following list represents a specific configuration status:
	| Bit [0]: Cohort Exists. Indicates that a cohort exists in PaCo.
	| Bit [1]: Codef Valid. Indicates thtat the Codef of a cohort is valid.
	| Bit [2]: Minimal Configuration Valid. Cohort exists in PaCo, Codef is valid, and configuration has been functionally validated.
	| Bit [3]: Retention Flag. Controls how a cohort will boot after a wakeup request.
	| Bit [4]: Cohort Locked. Indicates that the cohort is locked.
	| Bit [5]: Cohort SAM Locked. Indicates that the cohort is locked by the Safety Manager.
	| Bit [6]: Cohort Wait Loading. Indicates that the cohort is waiting to start a loading operation.

uint32 runtime_faults:
	Each bit in the following list represents a specific fault condition:
	| Bit [0]: Safety Fault Detected. Indicates that a safety-related fault has been detected.
	| Bit [1]: System Fault Detected. Indicates that a general system fault has been detected.
	| Bit [2]: Open Cohort Fault. A fault occurred while attempting to open a cohort.
	| Bit [3]: User Stubs Fault. Indicates a fault related to user stubs.
	| Bit [4]: Apply PaCo for Subsystem Fault. A fault occurred while applying PaCo for a subsystem.
	| Bit [31]: Shutdown Fault. A critical fault was encountered when a cohort could not be shut down.

uint32 cohort_state:
	Cohort state. The possible values are:
	| 0x00: Invalid. Cohort has an invalid state (either not configured or misconfigured).
	| 0x01: Shutdown. Cohort is shut down (not powered, all resources in reset).
	| 0x02: Stopped. Cohort is stopped (resources are in reset).
	| 0x03: Prepared. Cohort is powered up and opened.
	| 0x04: Images Loading. Cohort images are currently loading.
	| 0x05: Images Loaded. Cohort images have been successfully loaded.
	| 0x06: Running. Cohort is running.
	| 0x07: Suspended. Cohort is suspended.
	| 0x08: Error. The boot process encountered an unrecoverable error.
	| 0x09: Shutdown Started. The cohort shutdown process has started.
	| 0x0A: Warm Reset Triggered. A warm reset process has started for the cohort.
	| 0x0B: Suspend Started. The cohort suspend process has started.

uint32 primary_load_error:
	Load error of the primary status. The possible values are:
	| 0: No error detected.
	| 1: Plain Transfer Fault. Error detected when transferring an image from the NVM environment to RAM.
	| 2: SMR Transfer Fault. Error detected when transferring a secure memory region.
	| 3: SMR Verify Fault. Error detected when validating a secure memory region.
	| 4: Alignment Fault. Memory is not boundary aligned.
	| 5: Device Configuration Fault. The device is not mapped.
	| 6: Illegal Address Range. Invalid source or destination address (inside FSS FIRMWARE TCM Range).
	| 7: Plain Transfer Timeout. Timeout occurred during image transfer from NVM to RAM.
	| 8: Load Interrupted. Loading was interrupted by another task.
	| 9: Invalid Environment. Boot environment is invalid.
	| 10: SD Transfer Fault. Error during SD transfer.
	| 11: Load Lock Not Granted. Access to loading memory lock was not granted.

uint32 primary_index:
	Index of the primary status used for transfer/security faults.

uint32 primary_core_error:
	Core error of the primary status. The possible values are:
	| 0: No error detected.
	| 1: Prepare Fault. Error encountered while attempting to release the core from reset.
	| 2: Verify Fault. Verification failure when attempting to start the core with an incorrect address.

utin32 backup_load_error:
	Load error of the backup status. The possible values are the same as those defined for the load error of the primary status.

utin32 backup_index:
	Index of the backup status used for transfer/security faults.

utin32 backup_core_error:
	Core error of the backup status. The possible values are the same as those defined for the core error of the primary status.

GET_RESET_REASON_INFO
~~~~~~~~~~~~~~~~~~~~~
This command allows the calling agent to request the reset reason information synchronously.

---------------------------

| message_id: 0xD
| protocol_id: 0x81

---------------------------

Parameters:

uint32 cohort_id:
	Target Cohort ID which receives the requested service.

---------------------------

Return values:

int32 status:
	One of, but not limited to, the following:
	- ``SUCCESS`` in case the reset reason of the target cohort has been returned correctly.
	- ``DENIED`` in case the requested cohort does not have permission to get the reset reason for the target cohort.
	- ``NOT_FOUND`` in case the target cohort ID does not exist.

	See section 4.1.4 of the SCMI specification for status code definitions.

uint32 reset_reason:
	Reset reason information in the following format:
	| Bits [31:16]: Requesting cohort ID
	| Bits [15:8]: Software entity. Possible values include:
		| 0x1: Another Cohort ID requested the reset.
		| 0x11: Initial Boot sequence will start the Cohort
		| 0x21: Safety Manager has requested a reset of the Cohort
		| 0x31: Security Manager has requested a reset of the Cohort
	| Bits [7:0]: Reset type. Possible values include:
		| 0x0: Unknown Reset / Initial Value
		| 0x1: Boot Cohort
		| 0x2: Cold Reset
		| 0x3: Warm Reset
		| 0x4: Wake-up
		| 0x13: Warm Reset was escalated to Cold Reset
		| 0x14: Wake-up was escalated to Cold Reset

---------------------------

BTM_NOTIFY
~~~~~~~~~~
FSS platform provides notifications of events in the platform to an agent that has resgistered through this command.

---------------------------

| message_id: 0x14
| protocol_id: 0x81

---------------------------

Parameters:

uint32 enable:
	If this value is 0, the platform does not send any BOOT_NOTIFY_EVENT messages to the calling agent. If this value is 1, the platform sends BOOT_NOTIFY_EVENT messages to the calling agent when an event is detected.

---------------------------

Return values:

int32 status:
	See section 4.1.4 of the SCMI specification for status code definitions.
