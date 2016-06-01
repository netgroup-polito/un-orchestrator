# DPDK-based VNF examples

This folder contains examples of network functions implemented as DPDK secondary
processes.

## How to install the vswitch

In order to use a DPDK-based VNFs it is necessary to install a proper DPDK enabled virtual Switch,
currenty the un-node supports xDPd and Open vSwitch, please refer to
[Install the proper virtual switch](../../orchestrator/README_COMPILE.md#install-the-proper-virtual-switch)
in order to get instructions about the installation.

## How to start the vswitch

Before running the un-node it is necessary to start the virtual switch, please read
[How to start the proper virtual switch](../../orchestrator/README_RUN.md#how-to-start-the-proper-virtual-switch)

## How to create your VNFs

Please check individual README's in each sub-package.
Those files will give you the instruction to create the binary for the selected VNF.
Once you have that binary, you can pass the link to the UN (by writing the appropriate entry in the name resolver configuration file) in order to instantiate it in your running environment.

Please note that the command line of DPDK processes, in order to be managed through the un-orchestrator, must be the following:

	$ sudo ./vnfname -c $coremask -n $memchannels --proc-type=secondary -- p --$port1
		... --p portN --s $s --l $log

where:

  * `$coremask` indicates the cores to be assigned to the application (note that this parameter is required by DPDK)
  * `$memchannels` is the number of memory channels (note that this parameter is required by DPDK)
  * `$port1 ... $portN` are the ports to be used by the application
  * `$s` useless. It will be removed soon
  * `$log` name of the file to be used by the application to print log information

**Warning:** xDPd and OvS-DPDK use different names for the rte_rings associated with a port,
hence your application must be properly written according to the vSwitch you plan to use:

  * *xDPd*: $portname`-to-nf` to be used by the application to receive packets,
			and $portname`-to-xdpd` to be used by the application to send packets.
  * *OvS-DPDK*: $portname`_tx` to be used by the application to receive packets,
			and $portname`_rx` to be used by the application to send packets.

