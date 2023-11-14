Copyright 2023 NXP

SCMI PINCTRL Configuration Protocol v1.0
========================================

PINCTRL Overview
----------------

An SoC has pads that are exposed for connecting peripherals. Each pad has
an unique name associated to it (e.g. PA_00, PB_05). Internally, the SoC
can expose multiple signals (that is pins) to a given pad. Pinmuxing is
the process of specifying which pins should be connected to the pads.
The same hardware module can sometimes also control properties
(pinconfig) of a pad, for example enabling the Input/Output Buffer,
pull-up/pull-down bias resistor.

Protocol Overview
-----------------

This protocol is an extension of the `Arm System Control and Management
Interface
(SCMI) <http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/index.html>`_.

It is part of the vendor extensions region of SCMI, with an ID of 0x80.
It allows the SCP to handle the pincontrol/pinmux/pinconf of the
platform on behalf of a requesting agent.

The commands for the protocol are split into four parts:
   * Standard SCMI messages
   * Pinctrl module information
   * Pinmux messages
   * Pinconf messages

Protocol Commands
-----------------

Standard SCMI messages
~~~~~~~~~~~~~~~~~~~~~~

PROTOCOL_VERSION
^^^^^^^^^^^^^^^^
On success, this command returns the version of the protocol. For this
version of the specification the return value must be 0x10000, which
corresponds to 1.0.

-----------------------------------

| message_id: 0x0
| protocol_id: 0x80

This command is mandatory.

-----------------------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

uint32 version:
   For this version of the specification the return value must be 0x10000.

-----------------------------------

PROTOCOL_ATTRIBUTES
^^^^^^^^^^^^^^^^^^^
This command returns the implementation details associated with this protocol.

-----------------------------------

| message_id: 0x1
| protocol_id: 0x80

This command is mandatory.

-----------------------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

uint32 attributes:
   | Bits[31:16]: reserved.
   | Bits[15:0]: the maximum number of pin ranges that the pinctrl hardware exposes. See `PINCTRL_DESCRIBE`_.

-----------------------------------

PROTOCOL_MESSAGE_ATTRIBUTES
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
On success, this command returns the implementation details associated
with a specific message in this protocol. In addition to the standard
status codes described in section 4.1.4 of the SCMI specification, the
command can return the error ``NOT_FOUND`` if the message identified by the
``message_id`` is not provided by the implementation.

-----------------------------------

| message_id: 0x2
| protocol_id: 0x80

-----------------------------------

Parameters:

uint32 message_id:
   message_id of the message.

-----------------------------------

Return values:

int32 status:
   ``NOT_FOUND`` if the message identified by ``message_id`` is not provided by
   the implementation or one of the status code definitions described by
   section 4.1.4 of the SCMI specification.

uint32 attributes:
   Flags that are associated with a specific command in the protocol.
   For all commands in this protocol, this parameter has a value of 0.

-----------------------------------

Pinctrl module information
~~~~~~~~~~~~~~~~~~~~~~~~~~

PINCTRL_DESCRIBE
^^^^^^^^^^^^^^^^
This message returns information about the pins that the pinctrl module
can control.

-----------------------------------

| message_id: 0x3
| protocol_id: 0x80

-----------------------------------

Parameters:

uint32 range_index:
   The index of the first ``{start, no_pins}`` range to return. The first call should set it to 0 and then
   increase it by the number of ranges returned.

-----------------------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

uint32 no_ranges:
   The number of ranges returned in this message.

{uint16 start, uint16 no_pins}[N]:
   An array describing the pins that are available for pinmux/pinconfig.
   The number of elements in the array is returned by the `PROTOCOL_ATTRIBUTES`_ message.
   The array elements need to be sorted in ascending order by the ``start`` field. The
   pin ranges must not overlap.

-----------------------------------

Pinmux messages
~~~~~~~~~~~~~~~

PINMUX_GET
^^^^^^^^^^
This command retrieves the configured function of a pin.

-----------------------------------

| message_id: 0x4
| protocol_id: 0x80

This command is mandatory.

-----------------------------------

Parameters:

uint16 pin:
   This is the id of the pin whose function is to be returned.

-----------------------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

uint16 function:
   This is the currently configured function of a pin.
   The meaning of this value is platform specific.

-----------------------------------

PINMUX_SET
^^^^^^^^^^
This command configures the function for multiple pins given as parameter.

-----------------------------------

| message_id: 0x5
| protocol_id: 0x80

This command is mandatory.

-----------------------------------

Parameters:

uint32 no_pins:
   The number of pins to be configured

{uint16 pin, uint16 function}[N]:
   An array of size ``no_pins``. Each pin will be configured to the specified function.
   The meaning of this function is platform specific.

-----------------------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

Pinconf messages
~~~~~~~~~~~~~~~~

Pinconfig value meaning
^^^^^^^^^^^^^^^^^^^^^^^

When it comes to pinconfig, there are two types of properties to be
configured:

   * boolean properties which support values such as enable/disable.
      * Example: pull-up bias, open-drain enable.
   * integer/custom format values: these properties have a value that occupies multiple bits.
      * Example: Slew-Rate, Input Debounce time.

In general, the pinconfig of a pin is specified in three parts:
   * A mask identifying which properties should be set;
   * A value which contains the values corresponding to boolean type properties;
   * An array of values for integer/custom format values.

The mask is an ``uint32`` mask that specifies which properties will have
values present.
The following table describes the meaning of each bit from the mask:

+-----+---------------------+----------+------------------------------------------+
| Bit | Name                | Type     | Description                              |
+=====+=====================+==========+==========================================+
| 0   | PIN_CONFIG_BIAS\    | bool     | Set the pin to weakly latch, driving the |
|     | _BUS_HOLD           |          | last value on a tri-state bus. The       |
|     |                     |          | argument is ignored.                     |
+-----+---------------------+----------+------------------------------------------+
| 1   | PIN_CONFIG_BIAS\    | bool     | Disable pull-up/pull-down bias. This     |
|     | _DISABLE            |          | argument is ignored.                     |
+-----+---------------------+----------+------------------------------------------+
| 2   | PIN_CONFIG_BIAS\    | bool     | Set the pin to a high impedance mode,    |
|     | _HIGH_IMPEDANCE     |          | "high-Z"/"third-state". This argument is |
|     |                     |          | ignored.                                 |
+-----+---------------------+----------+------------------------------------------+
| 3   | PIN_CONFIG_BIAS\    | bool     | Enable a pull-down bias for the pin. If  |
|     | _PULL_DOWN          |          | the argument is 0, configure the         |
|     |                     |          | pull-down bias, if it is 1 connect the   |
|     |                     |          | pin to GND.                              |
+-----+---------------------+----------+------------------------------------------+
| 4   | PIN_CONFIG_BIAS\    | bool     | Configure the pin with the bias that the |
|     | _PULL_PIN_DEFAULT   |          | hardware controller decides. If the      |
|     |                     |          | argument is 0, this config is ignored,   |
|     |                     |          | otherwise the controller configures the  |
|     |                     |          | bias.                                    |
+-----+---------------------+----------+------------------------------------------+
| 5   | PIN_CONFIG_BIAS\    | bool     | Enable a pull-up bias for the pin. If    |
|     | _PULL_UP            |          | the argument is 0, configure the pull-up |
|     |                     |          | bias, if it is 1 connect it to a voltage |
|     |                     |          | source.                                  |
+-----+---------------------+----------+------------------------------------------+
| 6   | PIN_CONFIG_DRIVE\   | bool     | Configure the pin to have an open-drain. |
|     | _OPEN_DRAIN         |          | This will also enable the output buffer. |
|     |                     |          | The argument is ignored.                 |
+-----+---------------------+----------+------------------------------------------+
| 7   | PIN_CONFIG_DRIVE\   | bool     | Configure the pin to have an             |
|     | _OPEN_SOURCE        |          | open-source. The argument is ignored.    |
+-----+---------------------+----------+------------------------------------------+
| 8   | PIN_CONFIG_DRIVE\   | bool     | Configure the pin to have a push-pull    |
|     | _PUSH_PULL          |          | connection. This will also enable the    |
|     |                     |          | output buffer and disable open-drain if  |
|     |                     |          | it was enabled. The argument is ignored. |
+-----+---------------------+----------+------------------------------------------+
| 9   | PIN_CONFIG_DRIVE\   |multi-bit | The maximum current that the pin will    |
|     | _STRENGTH           |          | sink/source. The argument is the maximum |
|     |                     |          | current in mA.                           |
+-----+---------------------+----------+------------------------------------------+
| 10  | PIN_CONFIG_DRIVE\   |multi-bit | The maximum current that the pin will    |
|     | _STRENGTH_UA        |          | sink/source. The argument is the maximum |
|     |                     |          | current in uA.                           |
+-----+---------------------+----------+------------------------------------------+
| 11  | PIN_CONFIG_INPUT\   |multi-bit | Configure a debounce when reading inputs |
|     | _DEBOUNCE           |          | on this pin. The argument is the debouce |
|     |                     |          | time in usecs.                           |
+-----+---------------------+----------+------------------------------------------+
| 12  | PIN_CONFIG_INPUT\   | bool     | Enable the pin's input buffer. If the    |
|     | _ENABLE             |          | argument == 1, enable input, otherwise   |
|     |                     |          | disable it.                              |
+-----+---------------------+----------+------------------------------------------+
| 13  | PIN_CONFIG_INPUT\   |multi-bit | Enable the Schmitt-trigger for the pin.  |
|     | _SCHMITT            |          | The argument is a custom format for      |
|     |                     |          | configuring the threshold for            |
|     |                     |          | hysteresis.                              |
+-----+---------------------+----------+------------------------------------------+
| 14  | PIN_CONFIG_INPUT\   | bool     | Enable/Disable Schmitt-trigger for the   |
|     | _SCHMITT_ENABLE     |          | pin. If the argument != 0, enable the    |
|     |                     |          | Schmitt-trigger, otherwise disable it.   |
+-----+---------------------+----------+------------------------------------------+
| 15  | PIN_CONFIG_MODE\    |multi-bit | Configure the pin for low-power          |
|     | _LOW_POWER          |          | operation. If there are multiple power   |
|     |                     |          | modes, the argument is a custom format   |
|     |                     |          | for switching them. Otherwise 1 enables  |
|     |                     |          | low-power, 0 disables it.                |
+-----+---------------------+----------+------------------------------------------+
| 16  | PIN_CONFIG_MODE\    | bool     | Configure the pin for PWM.               |
|     | _PWM                |          |                                          |
+-----+---------------------+----------+------------------------------------------+
| 17  | PIN_CONFIG_OUTPUT   | bool     | Enable the output buffer and set a       |
|     |                     |          | value. If the argument is 1, set the     |
|     |                     |          | line to high level, if argument is 0,    |
|     |                     |          | set it to low level.                     |
+-----+---------------------+----------+------------------------------------------+
| 18  | PIN_CONFIG_OUTPUT\  | bool     | Enable the pin's output buffer without   |
|     | _ENABLE             |          | setting a value. The currently           |
|     |                     |          | configured function for a pin will drive |
|     |                     |          | the value. If the argument is 1, enable  |
|     |                     |          | the output buffer, otherwise disable it. |
+-----+---------------------+----------+------------------------------------------+
| 19  | PIN_CONFIG_PERSIST\ | bool     | Retain the pin's state across sleep      |
|     | _STATE              |          | modes/controller resets.                 |
+-----+---------------------+----------+------------------------------------------+
| 20  | PIN_CONFIG_POWER\   |multi-bit | Select from multiple available power     |
|     | _SOURCE             |          | sources for a pin. The argument is in a  |
|     |                     |          | custom format.                           |
+-----+---------------------+----------+------------------------------------------+
| 21  | PIN_CONFIG_SKEW\    |multi-bit | Configure the skew rate for inputs/latch |
|     | _DELAY              |          | delay for outputs. The argument is in a  |
|     |                     |          | custom format.                           |
+-----+---------------------+----------+------------------------------------------+
| 22  | PIN_CONFIG_SLEEP\   | bool     | Indicate this is a sleep state. The      |
|     | _HARDWARE_STATE     |          | argument is ignored.                     |
+-----+---------------------+----------+------------------------------------------+
| 23  | PIN_CONFIG_SLEW\    |multi-bit | Configure the slew rate for a pin. The   |
|     | _RATE               |          | argument is in a custom format.          |
+-----+---------------------+----------+------------------------------------------+
| 31- | Reserved            | N/A      | N/A                                      |
| 24  |                     |          |                                          |
+-----+---------------------+----------+------------------------------------------+

Note that Reserved bits must be set to 0.

If a bit in the mask is set to 1, it means that the value for the
property will be sent in the value present after the mask. The value can
be specified in the following ways:

* For boolean properties, in the ``boolean_values`` field at the same bit position;
* For multi-bit properties, in the ``configs`` array.

If there are multiple multi-bit properties specified in the mask, they will be present
in the array in the respective order that the properties are specified in the mask. The
order for parsing bits is MSB to LSB.

An example of this would be:

-  First uint32::

      +----------+-------+----------+-------+---------+-------+---------+
      | Bit 23   | ...   | Bit 12   | ...   | Bit 9   | ...   | Bit 3   |
      +==========+=======+==========+=======+=========+=======+=========+
      | 1        | ...   | 1        | ...   | 1       | ...   | 1       |
      +----------+-------+----------+-------+---------+-------+---------+

   All other bits are 0.

   This would make the pin an input, with a pull down enabled.
   Furthermore, there are two multi-bit properties to be read.

-  Second uint32: This value corresponds to the slew rate.
-  Third uint32: This value corresponds to the drive strength.

PINCONF_GET
^^^^^^^^^^^
This command retrieves the configured settings (e.g. pin bias) of a pin.

-----------------------------------

| message_id: 0x6
| protocol_id: 0x80

This command is mandatory.

-----------------------------------

Parameters:

uint16 pin:
   This is the id of the pin whose function is to be returned.

-----------------------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

uint32 configs_mask:
   This mask identifies all properties configured for this pin. For boolean
   properties the corresponding bit identifies whether the property is
   configured or not. For multi-bit properties, it identifies whether there
   is any value configured for this property. Note that if a property
   doesn't have the corresponding bit in the ``configs_mask`` set, it can
   be assumed that the property is disabled.

uint32 boolean_values:
   This are the values that correspond to boolean configs.

uint32 configs[N]:
   These are the multi-bit values configured. They are returned in the same
   order that their bits are in the ``configs_mask``.

-----------------------------------

PINCONF_SET_OVERRIDE
^^^^^^^^^^^^^^^^^^^^
This command is used to configure the parameters of one pin. All other
settings not present here are disabled/set to 0. For more details
regarding the format of pinconfig messages, see `Pinconfig value meaning`_
section.

-----------------------------------

| message_id: 0x7
| protocol_id: 0x80

This command is mandatory.

-----------------------------------

Parameters:

uint32 no_pins:
   The number of ``pins[N]`` present in this message.

uint16 pins[N]:
   These are the pins to be set.

uint32 mask:
   This is the mask which specifies which properties are to be changed.

uint32 bool_configs:
   This contains the configs to be applied for boolean properties.
   The bits corresponding to multi-bit properties are to be ignored.

uint32 multi_bit_configs[M]:
   This is an array of multi-bit configs to be applied for the pins. The order
   for the configs should match the bit order in the mask. The size M is
   the number of bits set to 1 that correspond to multi-bit properties.

-----------------------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

-----------------------------------

PINCONF_SET_APPEND
^^^^^^^^^^^^^^^^^^
This command is used to append the configuration of the given parameters
for one pin. All other settings not present here are unchanged. For more
details regarding the format of pinconfig messages, see `Pinconfig value meaning`_
section.

-----------------------------------

| message_id: 0x8
| protocol_id: 0x80

This command is mandatory.

-----------------------------------

Parameters:

uint32 no_pins:
   The number of ``pins[N]`` present in this message.

uint16 pins[N]:
   These are the pins to be configured.

uint32 mask:
   This is the mask which specifies which properties are to be changed.

uint32 bool_configs:
   This contains the configs to be applied for boolean properties.
   The bits corresponding to multi-bit properties are to be ignored.

uint32 multi_bit_configs[M]:
   This is an array of multi-bit configs to be applied for the pins. The order
   for the configs should match the bit order in the mask. The size M is
   the number of bits set to 1 that correspond to multi-bit properties.

-----------------------------------

Return values:

int32 status:
   See section 4.1.4 of the SCMI specification for status code definitions.

-----------------------------------

Linux pinctrl specifics
-----------------------

In Linux, pinctrl drivers are typically made out of 3 components:

   - pinctrl
   - pinmux
   - pinconfig

Specifying pins configuration is done in Linux via the ``.dts`` file inside
the pinctrl driver's node. Each subnode there defines a function (e.g.,
I2C4). Each function can have multiple groups, each group defining
unique pinconf settings.

Pinctrl is responsible for getting information about groups (which are
defined in the ``.dts``, multiple groups per peripheral).

Pinmux is responsible for getting information about functions and
setting a function for a group.

Pinconfig is responsible for configuring pins' properties (e.g., pin
bias).

S32CC particularities
~~~~~~~~~~~~~~~~~~~~~

SIUL2 is the module responsible for pinmux for the S32G SoC. The
configuration of a pad (i.e., connecting a signal to it, changing its
properties) is done via MSCRs/IMCRs.

A pin corresponds to the ``CR`` column inside the ``IO Signal Table`` from
the ``S32G_IOMUX.xlsx``. Physically, it can correspond to an actual pad
from the SoC (MSCR) or to some internal pin (IMCR). MSCRs can also have
pinconfig settings applied, while IMCRs cannot. The function of a pin is
the value of the ``SSS`` field and has a per-pin specific meaning.

Pinmuxing is done via writing the ``SSS`` field of a MSCR register and
the value of an IMCR register (if it's the case). This value of this
field represents the function for that pin. For more details check out
the ``IO Signal Table`` tab of the ``S32G_IOMUX.xlsx``.

Pinconfig is done by writing the other bits present from MSCRs (i.e.,
IBE/OBE, ODE).

Note that not all slew rates are possible. To see the supported values,
please refer to the ``S32G Reference Manual - SIUL2 chapter, MSCR register
description``.

The function of a pin is an integer number that is specific for each
pin. To see more details about what each number means, check out the
``S32G_IOMUX.xlsx`` attached to the ``S32G Reference Manual``.

