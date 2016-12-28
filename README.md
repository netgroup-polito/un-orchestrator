# Universal Node Repository Summary

[![Build Status](https://api.travis-ci.org/netgroup-polito/un-orchestrator.png)](https://travis-ci.org/netgroup-polito/un-orchestrator)

## Live playground

For who is interested to play with this software without having to install everything, a live playground is available [here](https://github.com/netgroup-polito/un-orchestrator/wiki/Live-Playground).

## Overview
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

An high-level overview of this software is given by the picture below.

![universal-node](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/universal-node.png)

The code of this module is available in the `orchestrator` folder; additional sub-modules are used to provide optional functions (e.g., graphical interface).
Please check individual README's in each subfolder for more information.



## Datastore
The datastore is an helper module that contains NF images and templates, NF-FGs, and more.
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

## Virtualizer
The Virtualizer is currently **deprecated**.
Please refer to the documention available in that folder for more information.

## NFs
This folder contains some examples of virtual network functions that are known to work on the UN.

## GUI
This folder contains a nice web-based GUI that allows to draw an NF-FG and deploy it on the UN, as well as to visualize NF-FGs already deployed.

## Use-cases
This folder contains some running use-cases for the UN, including configuration files and VNFs.
