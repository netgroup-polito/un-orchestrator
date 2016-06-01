# Universal Node Repository Summary
This repository contains the current implementation of the Universal Node and is divided in different sub-modules.
Please check individual README's in each subfolder.

An high-level overview of this software is given by the picture blow.

![universal-node](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/universal-node.png)


## Orchestrator
The Universal Node orchestrator (un-orchestrator) is the main component of the Universal Node (UN).
It handles the orchestration of compute and network resources within a UN, hence managing the complete lifecycle of computing containers (e.g., VMs, Docker, DPDK processes) and networking primitives (e.g., OpenFlow rules, logical switching instances, etc).

In a nutshell, when it receives a new Network Functions Forwarding Graph (NF-FG) to be deployed, it does the following operations:

  * retrieve the most appropriate images for the selected virtual network
    functions (VNFs) through the VNF name resolver;
  * configure the virtual switch (vSwitch) to create a new logical switching
    instance (LSI) and the ports required to connect it to the VNFs to be deployed;
  * deploy and start the VNFs;
  * translate the rules to steer the traffic into OpenFlow `flowmod` messages
    to be sent to the vSwitch (some `flowmod` are sent to the new LSI, others
    to the LSI-0, i.e. an LSI that steers the traffic towards the proper graph.)

Similarly, the un-orchestrator takes care of updating or destroying a graph,
when the proper messages are received.


## Name Resolver
The Name Resolver is a module that returns a set of implementations for a given NF.
It is exploited by the un-orchestrator each time that a NF must be started in order to translate the 'abstract' name (e.g., *firewall*) into the proper suitable software image (e.g., *firewall\_vmimage\_abc*).

## Virtualizer
The Virtualizer is a module that enables the un-orchestrator to interact with the upper layers of the Unify architecture, by means of the NF-FG defined in UNIFY. It in fact converts that NF-FG in the native representation accepted by the un-orchestrator.

The virtualizer operates as operates as follows:

  * it receives the NFFG commands through its northbound interface, based on the virtualizer library defined in UNIFY that implements the official NF-FG specification;
  * converts those commands in the NFFG formalism natively supported by the un-orchestrator;
  * through its southbound API, sends the equivalent command to the un-orchestrator.

This module is only required to integrate the un-orchestrator with the upper layers of the Unify architecture.
Instead, it is not needed when the un-orchestrator is controller through its native interface; in the case, the native NF-FG specification must be used.

## NFs
This folder contains some examples of virtual network functions that are known to work on the UN.
The `docker/elastic router` folder contains the source code of the Elastic Router VNFs.
These VNFs should be linked by the name-resolver.

## Use-cases
This folder contains some running use-cases for the UN, including configuration files and VNFs.
It contains the additional files to deploy different use-cases.
The `elastic-router` folder has all additional sources __other__ than the VNFs. 
Meaning containers or other files which are not being deployed by from the NFFG by the UN orchestrator.
It contains all containers, configuration files that are needed to deploy the elastic router 
use case and supporting functions for the troubleshooting and monitoring.

