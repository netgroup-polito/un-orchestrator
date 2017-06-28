This folder contains some examples of domain descriptions, written  according
to the [domain information library](https://github.com/netgroup-polito/domain-information-library).

The file [un-resource-description.json](un-resource-description.json) describes a single domain connected to an access network. the domain does not support GRE tunnels. Moreover, the domain has the `nat` functional capability.

The file [un-resource-description-with-neighbor-gre.json](un-resource-description-with-neighbor-gre.json) describes a domain 
is connected to another domain (called `openstack-domain`).
In this case, GRE tunnels can be used for the inter-domain traffic steering.
The interface name must be in the form `local_un_orchestrator_ip_address/interface_name` (i.e., 192.168.0.25/eth1). 
The remote interface must be in the form `another_domain_ip_address/interface_name` (i.e., 192.168.0.26/eth1).

The file [un-resource-description-with-neighbor-vlan.json](un-resource-description-with-neighbor-vlan.json) describes a domain 
is connected to another domain (called `openstack-domain`). In this case, the VLAN ID (i.e., 223) can be used for the inter-domain traffic steering.The interface name must be in the form `ip_address/interface_name` (i.e., of:000028c7ce9f66040/5104). 

If you are interested to add or modify some properties, please check the file [domain-information-library/data_model/di_data.json](https://github.com/netgroup-polito/domain-information-library/blob/master/data_model/di_data.json).
