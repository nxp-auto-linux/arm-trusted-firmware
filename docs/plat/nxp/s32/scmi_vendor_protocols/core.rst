Copyright 2025 NXP

SCMI NXP Core Protocol v1.0
===========================

Protocol Overview
-----------------
This protocol is an extension of the `Arm System Control and Management Interface (SCMI)
<http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/index.html>`_.

The goal of this protocol is to provide core management operations for the CPUs
part of a cohort.

It is added as a Vendor Extension to SCMI, with the ID of 0x83.
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
| protocol_id: 0x83

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
| protocol_id: 0x83

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
| protocol_id: 0x83

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

SCMI NXP Core Messages
-------------------

CONFIGURE_CORE
~~~~~~~~~~~~~~~
This command configures the reset vector address for the specified core.

---------------------------

| message_id: 0x3
| protocol_id: 0x83

This command is mandatory.

---------------------------

Parameters:

uint32 core_id:
   ID of the targeted core.

uint32 lower_entrypoint:
   Lower 32 bits of the reset vector address.

uint32 upper_entrypoint:
   Upper 32 bits of the reset vector address.

---------------------------

Return values:

int32 status:
   One of, but not limited to, the following
   - ``DENIED`` if the cohort making the request does not have permission for that core ID.
   - ``SUCCESS`` if the core has been correctly configured.

   See section 4.1.4 of the SCMI specification for status code definitions.
