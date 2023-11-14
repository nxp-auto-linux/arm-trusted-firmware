Copyright 2023 NXP

SCMI GPIO Configuration Protocol v1.0
=====================================

Protocol Overview
-----------------
This protocol is an extension of the `Arm System Control and Management
Interface (SCMI) <http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/index.html>`_.

The goal of this protocol is for the SCP to provide an interface to the SCMI agents for
the GPIO pins and GPIO IRQ control. The protocol provides 3 sets of commands:

* Protocol commands:

   * Describe the protocol version
   * Discover implementation attributes

* GPIO pins commands:

   * Describe a GPIO pin
   * Request or free a GPIO pin
   * Get or set value for a GPIO pin
   * Get direction of a GPIO pin

* GPIO IRQ commands:

   * Describe a GPIO IRQ
   * Request or free a GPIO IRQ
   * Set the type of a GPIO IRQ
   * Mask or unmask a GPIO IRQ

.. role:: raw-html(raw)
   :format: html

The protocol identifier used for this protocol (0x81) is within the range that the SCMI specification
provides for platform-specific extensions (0x80 - 0xFF). :raw-html:`<br/>`
For further information on protocol identifiers refer to section 4.1.2 of the SCMI specification.

Protocol Commands
-----------------

PROTOCOL_VERSION
~~~~~~~~~~~~~~~~
On success, this command returns the version of the protocol. For this
version of the specification the return value must be 0x10000, which
corresponds to 1.0.

-------------------

| message_id: 0x0
| protocol_id: 0x81

This command is mandatory.

-------------------

Return values:

int32 status:
  See section 4.1.4 of the SCMI specification for status code definitions.

uint32 version:
  For this version of the specification the return value must be 0x10000.

------------------

PROTOCOL_ATTRIBUTES
~~~~~~~~~~~~~~~~~~~
This command returns the implementation details associated with this protocol.

-------------------

| message_id: 0x1
| protocol_id: 0x81

This command is mandatory.

--------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

uint32 attributes:
      | Bits [31:16]: Number of GPIO IRQs
      | Bits [15:0]: Number of GPIO pins

--------------------

PROTOCOL_MESSAGE_ATTRIBUTES
~~~~~~~~~~~~~~~~~~~~~~~~~~~
On success, this command returns the implementation details associated
with a specific message in this protocol. In addition to the standard
status codes described in section 4.1.4 of the SCMI specification, the
command can return the error ``NOT_FOUND`` if the message identified by
``message_id`` is not provided by the implementation.

----------------------

| message_id: 0x2
| protocol_id: 0x81

This command is mandatory.

-----------------------

Parameters:

uint32 message_id:
   message_id of the message.

-----------------------

Return values:

int32 status:
   ``NOT_FOUND`` if the message identified by ``message_id`` is not provided by
   the implementation or one of the status code definitions described by
   section 4.1.4 of the SCMI specification.

uint32 attributes:
   Flags associated with a specific command in the protocol. For all commands
   in this protocol this parameter has a value of 0.

-----------------------

GPIO_DESCRIBE
~~~~~~~~~~~~~
This command allows the caller to get the available GPIO pins. In some
cases, some GPIO pins might be either reserved or not available.

-----------------------

| message_id: 0x3
| protocol_id: 0x81

This command is mandatory.

-----------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

uint32 base id:
   Bits[31:0]: Identifies the first GPIO number handled by this chip.

uint32 availabilities[N]:
   Availability array: Each bit of the array represents the availability of a GPIO pin.
   0 will be used to mark a GPIO as reserved/not available.

   The bit associated to a GPIO pin is encoded as::

      array element id = GPIO pin / 32
      bit of the element = GPIO pin % 32

   :Example:
      The bit 12 of the element 5 of the array will reflect the availability of the GPIO 172.

   **N** represents the number of ``uint32`` slots needed to accomodate the
   the number of GPIO pins obtained using `PROTOCOL_ATTRIBUTES`_ command.

-----------------------

GPIO_REQUEST
~~~~~~~~~~~~
This command requests the ownership of a GPIO pin. It must be called
before any other request on a pin. The direction of the GPIO pin is
expected to be set through the PINCTRL layers.

-----------------------

| message_id: 0x4
| protocol_id: 0x81

This command is mandatory.

-----------------------

Parameters:

uint32 GPIO pin:
   Identifier for the GPIO pin. It must be one of the pins marked as available by
   `GPIO_DESCRIBE`_ command.

-----------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

-----------------------

GPIO_FREE
~~~~~~~~~
This command releases the ownership of a GPIO pin obtained after a
`GPIO_REQUEST`_ command.

-----------------------

| message_id: 0x5
| protocol_id: 0x81

This command is mandatory.

-----------------------

Parameters:

uint32 GPIO pin:
   Identifier for the GPIO pin.

-----------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

-----------------------

GPIO_SET_VALUE
~~~~~~~~~~~~~~
Assign an output value for a GPIO pin (the direction of the pin was set
to output through the PINCTRL layers).

-----------------------

| message_id: 0x6
| protocol_id: 0x81

This command is mandatory.

-----------------------

Parameters:

uint32 GPIO pin:
   Identifier for the GPIO pin.

uint32 value:
   | 1 - drive the signal high
   | 0 - drive the signal low

-----------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

-----------------------

GPIO_GET_VALUE
~~~~~~~~~~~~~~
Return the value of a GPIO signal (the direction of the pin was set to
input through PINCTRL layers).

-----------------------

| message_id: 0x7
| protocol_id: 0x81

This command is mandatory.

-----------------------

Parameters:

uint32 GPIO pin:
   Identifier for the GPIO pin.

-----------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

uint32 value:
   | 1 - signal high
   | 0 - signal low

-----------------------

GPIO_GET_IRQ
~~~~~~~~~~~~
Return the IRQ number associated to a GPIO pin.

-----------------------

| message_id: 0x8
| protocol_id: 0x81

This command is mandatory.

-----------------------

Parameters:

uint32 GPIO pin:
   Identifier for the GPIO pin.

-----------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

uint32 IRQ number:
   The IRQ number associated to the provided GPIO pin.

-----------------------

:Example:
   External IRQ 6 is associated to GPIO 10.
   This command will return 6 in case the provided GPIO pin (parameter) is 10.

-----------------------

GPIO_IRQ_ENABLE
~~~~~~~~~~~~~~~
This command enables IRQ capability on an already requested GPIO pin.
The command will end with ``NOT_SUPPORTED`` error in case the passed GPIO
does not have IRQ capability.

-----------------------

| message_id: 0x9
| protocol_id: 0x81

This command is mandatory.

-----------------------

Parameters:

uint32 GPIO pin:
   Identifier for the GPIO pin.

uint32 sensitivity:
   | bit 0 - rising edge, configure rising edge if this bit is set.
   | bit 1 - falling edge, configure falling edge if this bit is set.

-----------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

-----------------------

GPIO_IRQ_DISABLE
~~~~~~~~~~~~~~~~
This command disables the IRQ capability of a GPIO pin obtained after a
`GPIO_IRQ_ENABLE`_ command.

-----------------------

| message_id: 0xa
| protocol_id: 0x81

This command is mandatory.

-----------------------

Parameters:

uint32 GPIO pin:
   Identifier for the GPIO pin.

-----------------------

Return values:

int32 status: See section 4.1.4 of the SCMI specification for status code definitions.

-----------------------

GPIO_IRQ_MASK
~~~~~~~~~~~~~
This command masks the source of the GPIO interrupt pin.

-----------------------

| message_id: 0xb
| protocol_id: 0x81

This command is mandatory.

-----------------------

Parameters:

uint32 GPIO identifier:
   Identifier for the GPIO.

-----------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

-----------------------

GPIO_IRQ_UNMASK
~~~~~~~~~~~~~~~
This command unmasks the source of the GPIO interrupt pin.

-----------------------

| message_id: 0xc
| protocol_id: 0x81

This command is mandatory.

-----------------------

Parameters:

uint32 GPIO identifier:
   Identifier for the GPIO.

-----------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

-----------------------

GPIO_GET_DIRECTION
~~~~~~~~~~~~~~~~~~

Return the direction of an already requested GPIO pin.

-----------------------

| message_id: 0xd
| protocol_id: 0x81

This command is mandatory.

-----------------------

Parameters:

uint32 GPIO pin:
   Identifier for the GPIO pin.

-----------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

uint32 direction:
   | 0 - output direction
   | 1 - input direction

-----------------------

Protocol Notifications
----------------------

GPIO_IRQ_NOTIFICATION
~~~~~~~~~~~~~~~~~~~~~
This is a notification to be sent by the SCMI platforms to the SCMI
agent that owns the GPIO interrupt. The platform will not mask the interrupt
before sending the notification.

-----------------------

| message_id: 0x0
| protocol_id: 0x81

This command is mandatory.

-----------------------

Parameters:

uint32 GPIO IRQ [N]:
   GPIO pending interrupts maskset: Each bit of the array represents the pending state of a GPIO IRQ line. 1
   will be used to mark a GPIO IRQ as pending.

   The bit associated to a GPIO IRQ is encoded as::

      array element id = GPIO IRQ / 32
      bit of the element = GPIO IRQ % 32

   :Example:
      The bit 13 of the element 5 of the array will reflect the pending state
      of the GPIO IRQ 173.

   **N** represents the number of ``uint32`` slots needed to accomodate the
   the number of GPIO interrupts obtained using `PROTOCOL_ATTRIBUTES`_ command.

---------------------

S32CC particularities
---------------------

All GPIO and GPIO IRQ pins are listed in ``S32G_IOMUX.xlsx``
spreadsheet, ``IO Signal Table`` sheet, as pins with GPIO or EIRQ
function. The spreadsheet is attached to the ``S32G Reference Manual``.
