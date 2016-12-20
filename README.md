# Universal Node Repository Summary

## Live playground

For who is interested to play with this software without having to install everything, a live playground is available [here](https://github.com/netgroup-polito/un-orchestrator/wiki/Live-Playground).


## Building status

[![Build Status](https://api.travis-ci.org/netgroup-polito/un-orchestrator.png)](https://travis-ci.org/netgroup-polito/un-orchestrator)

This repository contains the current implementation of the Universal Node and is divided in different sub-modules.
Please check individual README's in each subfolder.

An high-level overview of this software is given by the picture blow.

![universal-node](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/universal-node.png)


## Orchestrator
The Universal Node orchestrator (un-orchestrator) is the main component of the Universal Node (UN).
It handles the orchestration of compute and network resources within a UN, hence managing the complete lifecycle of computing containers (e.g., VMs, Docker, DPDK processes) and networking primitives (e.g., OpenFlow rules, logical switching instances, etc).

In a nutshell, when it receives a new Network Functions Forwarding Graph (NF-FG) to be deployed, it does the following operations:

  * retrieve the most appropriate images for the selected virtual network
    functions (VNFs) through the datastore;
  * configure the virtual switch (vSwitch) to create a new logical switching
    instance (LSI) and the ports required to connect it to the VNFs to be deployed;
  * deploy and start the VNFs;
  * translate the rules to steer the traffic into OpenFlow `flowmod` messages
    to be sent to the vSwitch (some `flowmod` are sent to the new LSI, others
    to the LSI-0, i.e. an LSI that steers the traffic towards the proper graph.)

Similarly, the un-orchestrator takes care of updating or destroying a graph,
when the proper messages are received.


## datastore
The datastore is a module that contains NF images and templates, NF-FGs, and more.
It is exploited by the un-orchestrator each time that a NF must be started, in 
order to:
  * retrive the NF template(s)
  * download the software image implementing the NF to be started
Particularly, in case the NF-FG indicates a specific template for the network 
function (e.g., *firewall_vmimage_abc*), only such a template is retrieved from 
the datastore. Otherwise the universal node orchestrastor downloads all the 
templates implementing the required function (e.g., *firewall*), and will
select one of them based on such internal policies.

The datastore can be installed either locally or on a remote server.

## Virtualizer - DEPRECATED
The Virtualizer is a module that enables the un-orchestrator to interact with the upper layers of the Unify architecture, by means of the NF-FG defined in UNIFY. It in fact converts that NF-FG in the native representation accepted by the un-orchestrator.

The virtualizer operates as follows:

  * it receives the NF-FG commands through its northbound interface, based on the virtualizer library defined in UNIFY that implements the official NF-FG specification;
  * converts those commands in the NF-FG formalism natively supported by the un-orchestrator;
  * through its southbound API, sends the equivalent command to the un-orchestrator.

This module is only required to integrate the un-orchestrator with the upper layers of the Unify architecture.
Instead, it is not needed when the un-orchestrator is controller through its native interface; in the case, the native NF-FG specification must be used.

**WARNING:** 

The virtualizer is deprecated; if you are interested in using it, you have to switch to the tag *virtualizer-working* through the following commands:

    $ cd [un-orchestrator]
    $ git checkout tags/virtualizer-working

## NFs
This folder contains some examples of virtual network functions that are known to work on the UN.

## GUI
This folder contains a nice web-based GUI that allows to draw an NF-FG and deploy it on the UN, as well as to visualize NF-FGs already deployed.

## Use-cases
This folder contains some running use-cases for the UN, including configuration files and VNFs.
