# Universal Node Orchestrator

The Universal Node orchestrator (un-orchestrator) is the main component of the
Universal Node (UN). It handles the orchestration of compute and network
resources within a UN, hence managing the complete lifecycle of computing
containers (e.g., VMs, Docker, DPDK processes) and networking primitives
(e.g., OpenFlow rules, logical switching instances, etc).
It receives commands through a REST API according to the Network Functions
Forwarding Graph (NF-FG) formalism, and takes care of implementing them on
the physical node.

More in detail, when it receives a command to deploy a new NF-FG, it does all
the operations required to actually implement the forwarding graph:

  * retrieve the most appropriate image for the selected virtual network
    function (VNF);
  * configure the virtual switch (vSwitch) to create a new logical switching
    instance (LSI) and the ports required to connect it to the VNF to be deployed;
  * deploy and start the VNF;
  * translate the rules to steer the traffic into OpenFlow flowmod messages
    to be sent to the vSwitch (some flowmod are sent to the new LSI, others
    to the LSI-0, i.e. an LSI that steers the traffic into the proper graph.)

Similarly, the un-orchestrator takes care of updating or destroying a graph,
when the proper messages are received.

As evident in the picture below, which provides an overall view of the UN, the
un-orchestrator includes several modules; the most important ones are the network
controller and the compute controller, which are exploited by the un-orchestrator itself to interact
respectively with the vSwitch and the hypervisor(s). These two modules are detailed in
the following.

![universal-node](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/universal-node.png)


### The network controller

The network controller is the sub-module that interacts with the vSwitch.
It consists of two parts:

  * the Openflow controller(s): a new Openflow controller is created for each
    new LSI, which is used to steer the traffic among the ports of the LSI
    itself;
  * the switch manager: it is used to create/destroy LSIs and virtual ports,
    and more. In practice, it allows the un-orchestrator to
    interact with the vSwitch in order to perform management operations. Each
    virtual switch implementation (e.g., xDPd, OvS) may require a different
    implementation for the switch manager, according to the API exported by
    the vSwitch itself.

Currently, the un-orchestrator supports OpenvSwitch (OvS) and the extensible DataPath daemon
(xDPd) as vSwitches.
If you are interested to add the support for a new virtual switch, please
check the file [network_controller/switch_manager/README.md](network_controller/switch_manager/README.md).

Note that, according to the picture above, several LSIs may be deployed on the UN.
In particular, in the boot phase the network controller creates a first LSI (called LSI-0)
that is connected to the physical interfaces; then, for each new graph to be deployed,
a new `tenant-LSI` is created and connected to the LSI-0 through a number of
internal links. Since each one of these additional LSIs corresponds to a different
NF-FG, it is connected to the VNFs of such a NF-FG, and takes care of steering the
traffic among them as required by the graph description. Instead the LSI-0, being
the only one connected to the physical interfaces of the UN and to all the other
graphs, dispatches the traffic entering into the node to the proper graph, and properly
handles the packets already processed in a graph.

### The compute controller

The compute controller is the sub-module that interacts with the hypervisor
and handles the lifecycle of a virtual network function (i.e., creating,
updating, destroying a VNF), including the operations needed to attach
VNF ports (already created on the the vSwitch) to the VNF itself. Each
execution environment may require a different implementation for the compute
controller, according to the commands supported by the hypervisor itself.

Currently the prototype supports virtual network functions as (KVM) VMs, Docker and DPDK
processes, although only a subset of them can be available depending on
the chosen vSwitch. If you are interested to add the support for a new
hypervisor, please check the file [compute_controller/README.md](compute_controller/README.md).

### Compute and network controllers: supported combinations

The following table shows which execution environments
are supported with the different vSwitches.

|                            |   Docker      |    KVM   |   KVM-DPDK (ivshmem)   |     DPDK processes     |
|----------------------------|---------------|----------|------------------------|------------------------|
| **xDPd-DPDK**              |    **Yes***   | **Yes*** |          No            |         **Yes**        |
| **OvS (OVSDB / OFconfig)** |    **Yes**    | **Yes**  | No (requires OvS-DPDK) | No (requires OvS-DPDK) |
| **OvS-DPDK**               |    **Yes***   | **Yes**  |        **Yes**         |         **Yes**        |
| **ERFS**                   |    **Yes***   | **No**   |        **Yes**         |         **Yes**        |

\* In this case the packet exchange between the virtual switch and the execution
environment is not optimized.

### NF-FG

The un-orchestrator supports two NF-FG versions:

  * the initial JSON-based format defined in WP5 and used in the initial
    part of the project;
  * the new XML-based format defined in WP3 that includes both top-down
    communication (for the actual forwarding graph) and bottom-up primitives
    (for resources and capabilities).

The former format is supported natively, while the other requires setting
up an additional library as described in [README_COMPILE.md#nf-fg-library](README_COMPILE.md#nf-fg-library).


### Compile and run

Some additional files are provided to compile and use the un-orchestrator:

  * [README_COMPILE.md](README_COMPILE.md): to compile the un-orchestrator
  * [README_RUN.md](README_RUN.md): to start the un-orchestrator
  * [README_NF-FG.md](README_NF-FG.md): description of the NF-FG formalism through examples
  * [README_RESTAPI.md](README_RESTAPI.md): some usage examples about the REST interface of
    the un-orchestrator
