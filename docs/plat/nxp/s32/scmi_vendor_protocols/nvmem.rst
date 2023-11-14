Copyright 2023 NXP

SCMI NVMEM Configuration Protocol v1.0
======================================

NVMEM Overview
--------------

NVMEM stands for Non-Volatile Memory layer. It is useful when wanting to
retrieve data from non-volatile storage such as EEPROMs, eFuses, etc.,
that usually corresponds to SoC revision ID, MAC addresses, part numbers
and so on, which some device drivers need.

Protocol Overview
-----------------

This protocol is an extension of the `Arm System Control and Management
Interface (SCMI) <http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/index.html>`_.

The goal of this protocol is for the SCP to provide a simple interface
for the SCMI agents, such that they can use it to read an NVMEM cell.

.. role:: raw-html(raw)
   :format: html

It is added as a Vendor Extension to SCMI, with the ID of 0x82. The ID
is within the range that the SCMI specification provides for
platform-specific extensions (0x80 - 0xFF).  :raw-html:`<br/>`
For further information on protocol identifiers refer to section 4.1.2 of the SCMI specification.

The commands for the protocol are split into two parts:
   * Protocol commands
   * NVMEM messages

Protocol Commands
-----------------

PROTOCOL_VERSION
~~~~~~~~~~~~~~~~
On success, this command returns the version of the protocol. For this
version of the specification the return value must be 0x10000, which
corresponds to 1.0.

---------------------------

| message_id: 0x0
| protocol_id: 0x82

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
| protocol_id: 0x82

This command is mandatory.

---------------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

uint32 attributes:
   Number of NVMEM cells managed by the platform.

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
| protocol_id: 0x82

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

SCMI NVMEM Messages
-------------------

NVMEM_READ_CELL
~~~~~~~~~~~~~~~
This command reads and returns the value stored in an NVMEM cell,
specified by a given offset and size.

---------------------------

| message_id: 0x3
| protocol_id: 0x82

This command is mandatory.

---------------------------

Parameters:

uint32 offset:
   Offset for the NVMEM cell, as defined by the platform.

uint32 bytes:
   Size (in bytes) of the requested read operation.

---------------------------

Return values:

int32 status:
  See section 4.1.4 of the SCMI specification for status code definitions.

uint32 bytes_read:
   Number of bytes read by the command. This value should be checked by the
   agent to correspond with the requested number of bytes.

uint32 value:
   Value read from the specified NVMEM cell.

---------------------------

NVMEM_WRITE_CELL
~~~~~~~~~~~~~~~~
This command writes a value to an NVMEM cell, specified by a given
offset and size.

---------------------------

| message_id: 0x4
| protocol_id: 0x82

This command is mandatory.

---------------------------

Parameters:

uint32 offset:
   Offset for the NVMEM cell, as defined by the platform.

uint32 bytes:
   Size (in bytes) of the requested write operation.

uint32 value:
   Value to be written to the specified NVMEM cell, with the bytes
   in little-endian format. The unused bytes are ignored.

---------------------------

Return values:

int32 status:
  See section 4.1.4 of the SCMI specification for status code definitions.

uint32 bytes_written:
   Number of bytes written by the command. This value should be checked by
   the agent to correspond with the requested number of bytes.

---------------------------

S32CC particularities
---------------------

The NVMEM cells export system configuration or control data, that is
stored in the MMIO registers of system hardware modules such as SIUL2,
MC_ME, etc. Since this protocol "flattens" the addressing scheme, by
considering all NVMEM cells as children of the main ``/firmware/scmi/nvmem_scmi``
node, the cells' offsets should be unique and kept in a translation
layer known to and synchronized between the platform and agents.
