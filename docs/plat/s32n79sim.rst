NXP S32N79 SIMULATOR
====================

S32N79 is a real-time processor for multi-SoC domain control applications, targeted for
multi-application ASIL D control through advanced real-time processing, hardware isolation and
virtualization. The S32N79 SoC is equipped with 8x ARM Cortex-A78AE, grouped in 4 clusters of
2x cores, that run at 1.8GHz. The Application Processor Subsystem (APS) consists of one cluster
and its associated peripherals. Clusters can be independently assigned to cohorts.

TF-A includes support for the S32N79's APS simulation model. Multiple TF-A instances can run
simultaneously, each of them having one or more APS under ownership.

How to build
------------

.. code:: shell

    make PLAT=s32n79sim clean
    make PLAT=s32n79sim BL33=<path/to/u-boot-nodtb.bin>

S32N79 configurable build options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Some build options can be used to control the current TF-A instance topology:
*   `PLATFORM_CLUSTER_COUNT`: Specifies the number of A78 clusters in a cohort or under current TF-A instance ownership.
Maximum value is 4.
*   `S32N_PLAT_PRIMARY_CPU`: Specifies the logical ID of the primary core, as computed by `plat_core_pos_by_mpidr`.

The memory allocated to a TF-A instance is configurable, as well as the location of the firmware components:
*   `DDR_START`: Specifies the starting address of the DRAM allocated to the current TF-A instance.
*   `DDR_END`: Specifies the ending address of the DRAM allocated to the current TF-A instance.
*   `PLAT_BL_MEM_SIZE`: Specifies the memory reserved for loading the BL2 and BL31 stages.
*   `S32N_FIP_DDR_OFFSET` : The offset in DDR of a preloaded FIP. Will be ignored if S32N_FIP_BASE
  default value is overridden.
*   `S32N_FIP_BASE` : The location in DDR of a preloaded FIP. By default computed as `DDR_START` + `S32N_FIP_DDR_OFFSET`.

When U-Boot (or other bootloader) is used as BL33, the following build options can be used:
*   `BL33_ENTRYPOINT_DDR_END_OFFSET`: Configures the entrypoint offset in DDR, relative to the end
  of it, for the normal world. Will be ignored if BL33_ENTRYPOINT default value is overridden.
*   `BL33_ENTRYPOINT`: Configures the entrypoint address for the normal world. By default computed
  as `DDR_END` - `BL33_ENTRYPOINT_DDR_END_OFFSET`.

Otherwise, if Linux Kernel (or other OS) image is used as BL33, the following build options can be used:
*   `BL33_ENTRYPOINT_DDR_START_OFFSET`: Configures the entrypoint offset in DDR, relative to the
  start of it, for the normal world (default: 0x0100_0000). Will be ignored if BL33_ENTRYPOINT default
  value is overridden.
*   `BL33_ENTRYPOINT`: Configures the entrypoint address for the normal world. By default computed
  as `DDR_START` + `BL33_ENTRYPOINT_DDR_START_OFFSET`.
*   `RAMDISK_RAM_OFFSET` : Root filesystem offset in RAM (default: 0x1400_0000).

Other customizable options:
*   `S32N_PL011_MODULE`: Selects the UART instance to be used by PL011 console driver. Options: 0 to 7.
*   `S32N_BOOT_SRC`: Selects the storage from where the FIP image should be loaded. The available options are: ufs, mmc and ddr.
*   `S32N_LINUX_KERNEL_AS_BL33` : Use the Linux Kernel Image as BL33, skipping U-Boot in the boot process.
*   `APS_PERIPH_BASE` : The configurable SSID part - bits [31:20] from the Generic Peripheral Mapping scheme (default: 0x40000000).

SCMI communication parameters:
*   `S32N_USE_FSS`: Controls whether FSS communication is enabled. Disabled by default.
*   `S32N_SCMI_MBX`: Selects the SCMI mailbox to be used for the communication between APS and FSS. Options: 0 to 3.
*   `FSS_SCMI_SH_MEM_<n>`: Configures the base address of the SCMI shared memory area. Use the `s32n_fconf_scmi_getter.c` file to update it.
*   `S32N_SCMI_MBX_SIZE`: Selects the size in bytes of the APS-FSS SCMI mailbox. Default value is 0x80, maximum value is 0xf00. Use the `s32n_fconf_scmi_getter.c` file to update it.
*   `FSS_MRU_CH`: Selects the FSS MRU channel used for communication. Available options: 1 to 15. Use the `s32n_fconf_scmi_getter.c` file to update it.
