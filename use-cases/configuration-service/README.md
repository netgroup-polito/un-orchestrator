Configuration service
=================

The following use-case supposes to be deployed in an architecture supporting the `configuration service` of the following repository:
https://github.com/netgroup-polito/generic-nfv-configuration-and-management

The goal of the configuration service is providing a dynamic way to push configurations into VNFs by means of an user-friendly GUI.

The tools provided in the use-case are:

 - Dockerfiles used to build `configuration VNFs` images, each VNF includes an agent able to communicate with the configuration service through a management network
 - The management network service graph
 - The service graph describing the following scenario
 
 ![universal-node](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/configuration_demo_scenario.png)

The scenario was thought for  a simple demonstration which shows how to push configuration into the VNFs in order to allow an user to gain an Internet connection.



