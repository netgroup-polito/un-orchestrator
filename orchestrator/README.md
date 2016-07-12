# Universal Node Orchestrator

The Universal Node orchestrator (un-orchestrator) is the main component of the
Universal Node (UN). It handles the orchestration of compute and network
resources within a UN, hence managing the complete lifecycle of computing
containers (e.g., VMs, Docker, DPDK processes) and
networking primitives (e.g., OpenFlow rules, logical switching instances, etc).
It receives commands through a REST API according to the Network Functions 
Forwarding Graph (NF-FG) formalism, and takes care of implementing them on 
the physical node. 

More in detail, when it receives a command to deploy a new NF-FG, it does all
the operations required to actually implement the forwarding graph:

  * retrieve the most appropriate images for the selected virtual network
    functions (VNFs);
  * configure the virtual switch (vSwitch) to create a new logical switching
    instance (LSI) and the ports required to connect it to the VNFs to be deployed;
  * deploy and start the VNFs;
  * translate the rules to steer the traffic into OpenFlow `flowmod` messages
    to be sent to the vSwitch (some `flowmod` are sent to the new LSI, others
    to the LSI-0, i.e. an LSI that steers the traffic towards the proper graph.)

Similarly, the un-orchestrator takes care of updating or destroying a graph,
when the proper messages are received.

As evident in the picture below, which provides an overall view of the UN, the
un-orchestrator includes several modules; the most important ones are the *network
controller* and the *compute controller*, which are exploited to interact
respectively with the vSwitch and the hypervisor(s).

The *VNF-selector* selects instead the best implementation for the required VNFs, according to some parameters such as the amount of CPU and RAM available on the Universal Node, or the efficiency of the network ports supported by the VNF itself (e.g., standard virtio vs. optimized ports). 
Moreover, the *VNF scheduler* optimizes the binding VNF/CPU core(s) by taking into account information such as how a VNF interacts with the rest of the NF-FG.

While the network and compute controllers are detailed in the following sections, the VNF-optimizer and the placement-optimizer have not been implemented yet, as their implementation is let as a future work.

![universal-node](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/universal-node.png)


### The network controller

The network controller is the sub-module that interacts with the vSwitch.
It consists of two parts:

  * the Openflow controller(s): a new Openflow controller is created for each
    new LSI, which steers the traffic among the ports of the LSI itself;
  * the switch manager: it creates/destroys LSIs, virtual ports,
    and more. In practice, it allows the un-orchestrator to
    interact with the vSwitch in order to perform management operations. Each
    virtual switch implementation (e.g., xDPd, OvS) may require a different
    implementation for the switch manager, according to the API exported by 
    the vSwitch itself.

Currently, the un-orchestrator supports OpenvSwitch (OvS), the extensible DataPath daemon
(xDPd) and the Ericsson Research Flow Switch (ERFS) as vSwitches, although further vSwiches can be supported by 
writing a module implementing a proper API.
If you are interested to add the support for a new virtual switch, please
check the file [network_controller/switch_manager/README.md](network_controller/switch_manager/README.md).

Note that, according to the picture above, several LSIs may be deployed on the UN. 
In particular, in the boot phase the network controller creates a first LSI (called LSI-0) 
that is connected to the physical interfaces and that will be connected to several other LSIs.
Each one of these additional LSIs corresponds to a different NF-FG; hence, it is connected to the VNFs 
of such a NF-FG, and takes care of steering the traffic among them as required by the graph description. 
Instead the LSI-0, being the only one connected to the physical interfaces of the UN and to all the other 
graphs, dispatches the traffic entering into the node to the proper graph, and properly 
handles the packets already processed in a graph.

### The compute controller

The compute controller is the sub-module that interacts with the virtual execution 
environment(s) (i.e., the hypervisor) and handles the lifecycle of a Virtual Network 
Function (i.e., creating, updating, destroying a VNF), including the operations needed 
to attach VNF ports already created on the the vSwitch to the VNF itself. Each
execution environment may require a different implementation for the compute
controller, according to the commands supported by the hypervisor itself.

Currently, the prototype supports virtual network functions as (KVM) VMs, Docker,
DPDK processes and native functions, although only a subset of them can be
available depending on the chosen vSwitch.
Also in thuis case, further execution environments can be supported through the implementation
of a proper API. 
If you are interested to add the support for a new hypervisor, please check the
file [compute_controller/README.md](compute_controller/README.md).

### Compute and network controllers: supported combinations

The following table shows the execution environments that
are supported with the different vSwitches.

|                            |   Docker      |    KVM   |   KVM-DPDK (ivshmem)   |     DPDK processes     |  Native   |
|----------------------------|---------------|----------|------------------------|------------------------|-----------|
| **xDPd-DPDK**              |    **Yes***   | **Yes*** |          No            |         **Yes**        | **Yes**   |
| **OvS (OVSDB / OFconfig)** |    **Yes**    | **Yes**  | No (requires OvS-DPDK) | No (requires OvS-DPDK) | **Yes**   |
| **OvS-DPDK**               |    **Yes***   | **Yes**  |        **Yes**         |         **Yes**        | **Yes***  |
| **ERFS**                   |    **Yes***   | **No**   |        **Yes**         |         **Yes**        | **No**    |

\* In this case the packet exchange between the virtual switch and the execution
environment is not optimized.

### NF-FG

The un-orchestrator natively supports the deployment of NF-FGs described with initial 
JSON-based format defined in WP5 and used in the initial part of the project.

If you plan instead to use the new XML-based format defined in WP3 that includes both 
top-down communication (for the actual forwarding graph) and bottom-up primitives (for 
resources and capabilities), you have also to run the [virtualizer](../virtualizer/README.md).


### Compile and run

Some additional files are provided to compile and use the un-orchestrator:

  * [README_COMPILE.md](README_COMPILE.md): to compile the un-orchestrator
  * [README_RUN.md](README_RUN.md): to start the un-orchestrator
  * [README_NF-FG.md](README_NF-FG.md): description of the NF-FG formalism through examples
  * [README_RESTAPI.md](README_RESTAPI.md): some usage examples about the REST interface of
    the un-orchestrator
  * [README_security.md](README_security.md): description of the security features of the un-orchestrator
  * [README_orchestrator_mode.md](README_orchestrator_mode.md): description of the operation modes of the un-orchestrator
