Copyright 2023-2024 NXP

SCP SCMI Device Tree Bindings
=============================

This document describes the device tree bindings used to define the
communication interface between the secure world software running on Cortex-A
and the Cortex-M System Control Processor (SCP), that is in charge of
handling various system management operations.

Properties
^^^^^^^^^^
Should be placed under ``firmware/scmi`` node.

- compatible [mandatory]:
    - Value type: ``<string>``
    - Must be "arm,scmi-smc".

- nxp,scp-mboxes [mandatory]:
    - Value type: ``<phandle>``
    - Phandles of shared memory nodes that describe the TX/RX/metadata/notification mailboxes.
      Each core uses its own TX mailbox and has an associated metadata memory zone (the metadata
      zones are disabled by default). In the SCMI split channels usecase, instead of using a TX mailbox for
      each core, the TX communication relies on two distinct mailboxes: a secure PSCI and a non-secure OSPM one.
      There is also an RX mailbox used for notifications, with its dedicated RX metadata memory and an OSPM
      notification mailbox which is used for communication with the non-secure world.

- nxp,scp-mbox-names [mandatory]:
    - Value type: ``<stringlist>``
    - The names of the mailboxes.

        - For TX mailboxes must be "scp_tx_mbX", where X represents the core ID
        - For RX mailbox must be "scp_rx_mb"
        - For TX metadata must be "scp_tx_mdX", where X represents the core ID
        - For RX metadata must be "scp_rx_md"
        - For OSPM notification mailbox must be "scmi_ospm_notif"
        - For SCMI split channels:

            - For PSCI mailbox must be "scp_tx_mb0"
            - For OSPM mailbox must be "scp_tx_mb1"
            - For PSCI metadata must be "scp_tx_md0"
            - For OSPM metadata must be "scp_tx_md1"
            - RX, metadata and OSPM notification mailboxes use the same names

- nxp,scp-irqs [mandatory]:
    - Value type: ``<prop-encoded-array>``
    - A list of (phandle, cpn, mscm_irq) tuples, describing the interrupts used for TX and RX communication,
      as well as the ones needed by PSCI and OSPM (TX and RX) channels, where:

      - phandle is the MSCM node phandle
      - cpn is an <u32> value representing the target core processor number
      - mscm_irq is an <u32> value representing the MSCM core-to-core interrupt

- nxp,scp-irq-names [mandatory]:
    - Value type: ``<phandle>``
    - Names of the TX and RX interrupts, including the interrupts used by the PSCI and OSPM channels.

        - For TX interrupt must be "scp_tx", mandatory in the default SCMI channels configuration
        - For RX interrupt must be "scp_rx", mandatory in the default SCMI channels configuration
        - For PSCI interrupt must be "scp_psci", mandatory in the SCMI split channels configuration
        - For OSPM interrupt must be "scp_ospm", mandatory in the SCMI split channels configuration
        - For OSPM RX interrupt must be "scp_ospm_rx",mandatory in the SCMI split channels configuration

- nxp,notif-irq [mandatory]:
    - Value type: ``<prop-encoded-array>``
    - A tuple of 3 <u32> values, describing the OSPM notification interrupt, where:

        - first <u32> represents the interrupt type: must be ``GIC_SPI``
        - second <u32> represents the GIC interrupt number
        - third <u32> represents the interrupt configuration: ``IRQ_TYPE_EDGE_RISING``

Mailbox nodes
`````````````
Should be placed in the ``reserved-memory`` node.

- no-map [mandatory]
- reg [mandatory]

Example
^^^^^^^
.. code:: devicetree

    #include <dt-bindings/interrupt-controller/arm-gic.h>
    #include <dt-bindings/mscm/s32cc-mscm.h>

    reserved-memory {
        #address-cells = <2>;
        #size-cells = <2>;

        scmi_scp_tx_mb0: shm@34500000 {
            reg = <0x0 0x34500000 0x0 0x80>;
            no-map;
        };

        scmi_scp_tx_mb1: shm@34500080 {
            reg = <0x0 0x34500080 0x0 0x80>;
            no-map;
        };

        scmi_scp_tx_mb2: shm@34500100 {
            reg = <0x0 0x34500100 0x0 0x80>;
            no-map;
        };

        scmi_scp_tx_mb3: shm@34500180 {
            reg = <0x0 0x34500180 0x0 0x80>;
            no-map;
        };

        scmi_scp_rx_mb: shm@34500200 {
            reg = <0x0 0x34500200 0x0 0x80>;
            no-map;
        };

        scmi_scp_tx_md0: shm@34500280 {
            reg = <0x0 0x34500280 0x0 0x80>;
            status = "disabled";
            no-map;
        };

        scmi_scp_tx_md1: shm@34500300 {
            reg = <0x0 0x34500300 0x0 0x80>;
            status = "disabled";
            no-map;
        };

        scmi_scp_tx_md2: shm@34500380 {
            reg = <0x0 0x34500380 0x0 0x80>;
            status = "disabled";
            no-map;
        };

        scmi_scp_tx_md3: shm@34500400 {
            reg = <0x0 0x34500400 0x0 0x80>;
            status = "disabled";
            no-map;
        };

        scmi_scp_rx_md: shm@34500480 {
            reg = <0x0 0x34500480 0x0 0x80>;
            status = "disabled";
            no-map;
        };

        scmi_ospm_notif: shm@d0000080 {
            reg = <0x0 0xd0000080 0x0 0x80>;
            no-map;
        };
    };

    firmware {
        scmi {
            nxp,scp-mboxes = <&scmi_scp_tx_mb0>, <&scmi_scp_tx_md0>,
              <&scmi_scp_tx_mb1>, <&scmi_scp_tx_md1>,
              <&scmi_scp_tx_mb2>, <&scmi_scp_tx_md2>,
              <&scmi_scp_tx_mb3>, <&scmi_scp_tx_md3>,
              <&scmi_scp_rx_mb>, <&scmi_scp_rx_md>,
              <&scmi_ospm_notif>;
            nxp,scp-mbox-names = "scp_tx_mb0", "scp_tx_md0",
              "scp_tx_mb1", "scp_tx_md1",
              "scp_tx_mb2", "scp_tx_md2",
              "scp_tx_mb3", "scp_tx_md3",
              "scp_rx_mb", "scp_rx_md",
              "scmi_ospm_notif";
			nxp,scp-irqs = <&mscm0 M7_0_CPN MSCM_C2C_IRQ_0>,
				<&mscm0 A53_0_CPN MSCM_C2C_IRQ_0>,
				<&mscm0 M7_0_CPN MSCM_C2C_IRQ_0>,
				<&mscm0 M7_0_CPN MSCM_C2C_IRQ_1>,
				<&mscm0 A53_0_CPN MSCM_C2C_IRQ_1>;
			nxp,scp-irq-names = "scp_tx", "scp_rx", "scp_psci", "scp_ospm",
				"scp_ospm_rx";
            nxp,notif-irq = <GIC_SPI 300 IRQ_TYPE_EDGE_RISING>;
        };
    };
