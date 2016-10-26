Frog use case
===================

This README details the FROG use case, that needs to work with the [service layer](https://github.com/netgroup-polito/frog4-service-layer) and the [global orchestrator](https://github.com/netgroup-polito/frog4-orchestrator). In this folder you can find all the configuration files and VNFs required.
The scenario is composed by four components:
> - **Fat CPE**:  a CPE with NFV capabilities;
> - **Slim CPE**:  a resource-constraint CPE (it doesn't run VNF);
> - **Generic server**: it contains the service layer, the global orchestrator and the DD broker;
> - **Data center**: Operator data center

The following picture illustrates the physical setup, with the initial graphs deployed by the service layer at the boot time.
![scenario](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/frog-setup-and-isp-graphs.PNG?raw=true)

As shown above, the initial graphs in the CPEs bring all the user traffic (not authenticated) to the `Authentication graph` in the ISP Data Center. `Web Captive Portal` and `OF Controller` collaborate togheter in order to recognize and authenticate the users. The good success of the authentication involve the instantiation of the `user graph`. All traffic always pass through the `ISP graph`, that gives IP addresses and allows traffic to reach internet.

![auth-graph](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/frog-auth-graph.PNG)
![isp-graph](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/frog-isp-graph.PNG)

If the user come from the slip CPE, wich can not run VNF due to its resources limitation, the own NFs that compose the `user graph` are deployed in the ISP datacenter and a simple rule that brings the user traffic to the proper graph of the datacenter is installed in the slim CPE.

![user-graph-remote-case](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/frog-user-graph-remote-case.PNG)

If the user is attached to the fat CPE, the VNFs of `user graph` are locally deployed and the traffic that pass through the VNFs will reach internet by means of the `ISP graph`.

![user-graph-local-case](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/frog-user-graph-local-case.PNG)

Different combinations are possible. For instance, user 1 can arrive from the slimCPE involving the deployment of the own `user graph` in the datacenter, after he moves to the FatCPE with an other device and athenticate himself as user1: In this case, the proper `user graph` is already instantiated, so will be created a simple rule in the FatCPE that brings the new traffic to the proper graph in the datacenter.
