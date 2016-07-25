# NF-FG examples
This section provides some examples of NF-FGs that can be deployed on the Universal
Node through the un-orchestrator.

Further examples are available in the [./config](./config) folder and at [https://github.com/netgroup-polito/nffg-library/blob/master/README_NFFG](https://github.com/netgroup-polito/nffg-library/blob/master/README_NFFG).

## Example 1

This example is very simple: configures a graph that receives all the traffic
from interface `eth1` and sends it to interface `eth2`, without traversing any
VNF.

	{
		"forwarding-graph": 
		{
			"id": "00000001",
			"name": "Forwarding graph",
			"end-points": [
		  	{
		    	"id": "00000001",
		    	"name": "ingress",
		    	"type": "interface",
		    	"interface": {
		      		"if-name": "eth1"
		    	}
		  	},
		  	{
		    	"id": "00000002",
		    	"name": "egress",
		    	"type": "interface",
		    	"interface": {
		      		"if-name": "eth2"
		    	}
		  	}
			],
			"big-switch": {
		  		"flow-rules": [
		    	{
		      	    "id": "00000001",
		      		"priority": 1,
		      		"match": {
		        		"port_in": "endpoint:00000001"
		      		},
		      		"actions": [
		        	{
		          		"output_to_port": "endpoint:00000002"
		        	}
		      		]
		    	}
		  		]
			}
	  	}
	}
	
## Example 2

This example is more complex, and it includes a network function called `firewall`.
Packets coming from the interface `eth1` are sent to the first port of the network
function (`firewall:inout:0`), while packets coming from the second port of the network
function (`firewall:inout:1`) are sent on the network interface `eth2`.

	{
		"forwarding-graph": 
		{
			"id": "00000001",
			"name": "Forwarding graph",
			"VNFs": [
		  	{
		    	"id": "00000001",
		    	"name": "firewall",
        		"ports": [
          		{
            		"id": "inout:0",
            		"name": "data-port"
          		},
          		{
            		"id": "inout:1",
            		"name": "data-port"
          		}
        		]
		  	}
			],
			"end-points": [
		  	{
		    	"id": "00000001",
		    	"name": "ingress",
		    	"type": "interface",
		    	"interface": {
		      		"if-name": "eth1"
		    	}
		  	},
		  	{
		    	"id": "00000002",
		    	"name": "egress",
		    	"type": "interface",
		    	"interface": {
		      		"if-name": "eth2"
		    	}
		  	}
			],
			"big-switch": {
		  		"flow-rules": [
		    	{
		      		"id": "00000001",
		      		"priority": 1,
		      		"match": {
		        		"port_in": "endpoint:00000001"
		      		},
		      		"actions": [
		        	{
		        		"output_to_port": "vnf:00000001:inout:0"
		        	}
		      		]
		    	},
		    	{
		      		"id": "00000002",
		      		"priority": 1,
		      		"match": {
		        		"port_in": "vnf:00000001:inout:1"
		      		},
		      		"actions": [
		        	{
		          		"output_to_port": "endpoint:00000002"
		        	}
		      		]
		    	}
		  		]
			}
	  	}
	}
	
## Example 3

In this example, traffic coming from `eth1` is forwarded to the `firewall` through the port
`firewall:inout:0`. Then, traffic coming from the firewall (`firewall:inout:0`) is split based on the destination
TCP port. Packets directed to the TCP port 80 is provided to the web cache then to the NAT,
while all the other traffic is directly provided to the NAT. Finally, packets from `NAT:inout:1` leaves the
graph through the port `eth2`.

This graph can be graphically represented as follows:

    eth1 -> firewall -> if (tcp_dst == 80) -> web cache  -> nat  -> eth2
                        else \--------------------------/

Json description of the graph:

	{
		"forwarding-graph": 
		{
			"id": "00000001",
			"name": "Forwarding graph",
			"VNFs": [
		  	{
		    	"id": "00000001",
		    	"name": "firewall",
        		"ports": [
          		{
            		"id": "inout:0",
            		"name": "data-port"
          		},
          		{
            		"id": "inout:1",
            		"name": "data-port"
          		}
        		]
		  	},
		  	{
		    	"id": "00000002",
		    	"name": "nat",
        		"ports": [
          		{
            		"id": "inout:0",
            		"name": "data-port"
          		},
          		{
            		"id": "inout:1",
            		"name": "data-port"
          		}
        		]
		  	},
		  	{
		    	"id": "00000003",
		    	"name": "web-cache",
        		"ports": [
          		{
            		"id": "inout:0",
            		"name": "data-port"
          		},
          		{
            		"id": "inout:1",
            		"name": "data-port"
          		}
        		]
		  	}
			],
			"end-points": [
		  	{
		    	"id": "00000004",
		    	"name": "ingress",
		    	"type": "interface",
		    	"interface": {
		      		"if-name": "eth1"
		    	}
		  	},
		  	{
		    	"id": "00000005",
		    	"name": "egress",
		    	"type": "interface",
		    	"interface": {
		      		"if-name": "eth2"
		    	}
		  	}
			],
			"big-switch": {
		  		"flow-rules": [
		    	{
		      		"id": "00000001",
		      		"priority": 1,
		      		"match": {
		        		"port_in": "endpoint:00000004"
		      		},
		      		"actions": [
		        	{
		        		"output_to_port": "vnf:00000001:inout:0"
		        	}
		      		]
		    	},
		    	{
		      		"id": "00000002",
		      		"priority": 10,
		      		"match": {
		        		"port_in": "vnf:00000001:inout:1",
            			"ether_type": "0x800",
            			"protocol": "0x06",
            			"dest_port": "80"
		      		},
		      		"actions": [
		        	{
		          		"output_to_port": "vnf:00000003:inout:0"
		        	}
		      		]
		    	},
		    	{
		      		"id": "00000003",
		      		"priority": 1,
		      		"match": {
		        		"port_in": "vnf:00000001:inout:1"
		      		},
		      		"actions": [
		        	{
		        		"output_to_port": "vnf:00000002:inout:0"
		        	}
		      		]
		    	},
		    	{
		      		"id": "00000004",
		      		"priority": 1,
		      		"match": {
		        		"port_in": "vnf:00000003:inout:1"
		      		},
		      		"actions": [
		        	{
		          		"output_to_port": "vnf:00000002:inout:0"
		        	}
		      		]
		    	},
		    	{
		      		"id": "00000005",
		      		"priority": 1,
		      		"match": {
		        		"port_in": "vnf:00000002:inout:1"
		      		},
		      		"actions": [
		        	{
		          		"output_to_port": "endpoint:00000005"
		        	}
		      		]
		    	}
		  		]
			}
	  	}
	}
		
## Matches

Within the `match` element of the NF-FG description, the following fields are allowed
(all the values must be specified as strings):

	`port_in`
	`ether_type`
	`source_mac`
	`eth_src_mask`
	`dest_mac`
	`eth_dst_mask`
	`source_ip`
	`dest_ip`
	`source_port`
	`dest_port`
	`protocol`
	`vlan_id`
	`vlan_pcp`
	`ip_dscp`
	`ip_ecn`
	`ipv4_src`
	`ipv4_src_mask`
	`ipv4_dst`
	`ipv4_dst_mask`
	`sctp_src`
	`sctp_dst`
	`icmpv4_type`
	`icmpv4_code`
	`arp_opcode`
	`arp_spa`
	`arp_spa_mask`
	`arp_tpa`
	`arp_tpa_mask`
	`arp_sha`
	`arp_tha`
	`ipv6_src`
	`ipv6_src_mask`
	`ipv6_dst`
	`ipv6_dst_mask`
	`ipv6_flabel`
	`ipv6_nd_target`
	`ipv6_nd_sll`
	`ipv6_nd_tll`
	`icmpv6_type`
	`icmpv6_code`
	`mpls_label`
	`mpls_tc`
        
## Actions

Within the `action` element of the NF-FG description, the following elements can be used:

	`output_to_port`
	`push_vlan`
	`pop_vlan`
  
Note that multiple actions can be specified in the same `flowrule`, and that `output_to_port` 
should appear **one and only one** in each `flowrule`.
  
As an example, the following NF-FG tags all the packets coming from interface `firewall:inout:1` 
(belonging to the `firewall` VNF) and forwards them to the `network-monitor` VNF, by means of its 
interface `network-monitor:inout:0`. Then, packets coming from `network-monitor:inout:1` are sent 
on the network through the physical port `eth1`, without any VLAN tag.

	{
		"forwarding-graph": 
		{
			"id": "00000001",
			"name": "Forwarding graph",
			"VNFs": [
		  	{
		    	"id": "00000001",
		    	"name": "firewall",
        		"ports": [
          		{
            		"id": "inout:0",
            		"name": "data-port"
          		},
          		{
            		"id": "inout:1",
            		"name": "data-port"
          		}
        		]
		  	},
		  	{
		    	"id": "00000002",
		    	"name": "network-monitor",
        		"ports": [
          		{
            		"id": "inout:0",
            		"name": "data-port"
          		},
          		{
            		"id": "inout:1",
            		"name": "data-port"
          		}
        		]
		  	}
			],
			"end-points": [
			{
		    	"id": "00000001",
		    	"name": "ingress",
		    	"type": "interface",
		    	"interface": {
		      		"if-name": "eth1"
		    	}
		  	}
			],
			"big-switch": {
		  		"flow-rules": [
		    	{
		      		"id": "00000001",
		      		"priority": 1,
		      		"match": {
		        		"port_in": "vnf:00000001:inout:1"
		      		},
		      		"actions": [
		      		{
		      			"push_vlan" : "0x25"
		      		},
		        	{
		        		"output_to_port": "vnf:00000002:inout:0"
		        	}
		      		]
		    	},
		    	{
		      		"id": "00000002",
		      		"priority": 1,
		      		"match": {
		        		"port_in": "vnf:00000002:inout:1",
		        		"vlan_id" : "0x25"
		      		},
		      		"actions": [
		      		{
		      			"pop_vlan" : true
		      		},
		        	{
		        		"output_to_port": "endpoint:00000001"
		        	}
		      		]
		    	}
		  		]
			}
	  	}
	}
	
Note that the flow-rule `00000002` does not specify any match on `"ether_type": "0x8100"`. In fact, you can use the `ether_type` 
match to indicate a match of the protocol encapsulated inside the VLAN header. For instance, the next flow-rule matches all the 
packets having VLAN ID equal to `0x25`, and with the IP header as a payload of the VLAN packet.

	{
		"id": "00000001",
		"priority": 1,
		"match": {
			"port_in": "vnf:00000001:inout:1",
			"vlan_id" : "0x25",
			"ether_type": "0x800"
		},
		"actions": [
		{
			"output_to_port": "vnf:00000002:inout:0"
		}
		]
	}


## Endpoints

The NF-FG specification supports several types of endpoints:

	`interface`
	`vlan`
	`gre-tunnel`
	`internal`

### Endpoint type: `interface`

It represents a physical interface of the UN. Its usage is shown in all the examples 
provided so far. 

### Endpoint type: `vlan`

It represents again an endpoint associated with a physical interface; however, the UN guarantees
that only the traffic with a specific VLAN ID enters from this endpoint, and that all the traffic 
exiting from such an endpoint will be tagged with such a VLAN ID.
For instance, the `vlan` endpoint can be used to connect together pieces of the same service 
deployed  on different Universal Nodes.

The `vlan` endpoint is defined as follows:

	{
		"id": "00000001",
		"name": "egress",
		"type": "vlan",
		"vlan": {
  			"vlan-id": "25",
  			"if-name": "eth1"
		}
	}

In this example, only the traffic coming from the physical interface `eth1` and tagged with VLAN ID 
`25` will enter from such an endpoint. Similarly, all the traffic that will exit from such an end point
will be sent through the physical interface `eth1` and tagged with the VLAN ID `25`. Note that all the 
operations needed to reproduce this behavior (e.g., push of the VLAN tag) are transparently implemented by 
the un-orchestrator.

A complete example that shows the usage of the `vlan` endpoint is the following, in which the endpoint is associated 
with the physical port `eth1` and with the VLAN ID `25`. The traffic coming from this endpoint is then provided to 
the `firewall` VNF, and then leaves the UN again through the `vlan` endpoint. This means that it will leave the 
UN through the physical interface `eth1` and then again tagged with the VLAN ID `25`.

	{
		"forwarding-graph": 
		{
			"id": "00000001",
			"name": "Only network graph",
			"VNFs": [
		  	{
		    	"id": "00000001",
		    	"name": "firewall",
        		"ports": [
          		{
            		"id": "inout:0",
            		"name": "data-port"
          		},
          		{
            		"id": "inout:1",
            		"name": "data-port"
          		}
        		]
		  	}
		  	],
			"end-points": [
		  	{
		    	"id": "00000001",
		    	"name": "egress",
		    	"type": "vlan",
		    	"vlan": {
		      		"vlan-id": "25",
		      		"if-name": "eth1"
		    	}
		  	}
			],
			"big-switch": {
		 		"flow-rules": [
		   	 	{
		      		"id": "00000001",
		      		"priority": 1,
		      		"match": {
		        		"port_in": "endpoint:00000001"
		      		},
		      		"actions": [
		        	{
		          		"output_to_port": "vnf:00000001:inout:0"
		        	}
		      		]
		    	},
		    	{
		      		"id": "00000002",
		      		"priority": 1,
		      		"match": {
		        		"port_in": "vnf:00000001:inout:1"
		      		},
		      		"actions": [
		        	{
		          		"output_to_port": "endpoint:00000001"
		        	}
		      		]
		    	}
		  	]
			}
	  	}
	}
	
#### How this endpoint is implemented by the un-orchestrator

Referring to the example above, the un-orchestrator changes the flowrule `00000001` as follows: 

 	{
  		"id": "00000001",
  		"priority": 1,
  		"match": {
    		"port_in": "endpoint:00000001",
    		"vlan_id": "0x25"
  		},
  		"actions": [
  		{
  			"pop_vlan" : true
  		},
    	{
      		"output_to_port": "vnf:00000001:inout:0"
    	}
  		]
	},
	
The flowrule `00000002` is instead changed as follows:

	{
  		"id": "00000002",
  		"priority": 1,
  		"match": {
    		"port_in": "vnf:00000001:inout:1"
  		},
  		"actions": [
  		{
  			"push_vlan" : "0x25"
  		},
    	{
      		"output_to_port": "endpoint:00000001"
    	}
  		]
	}
 
This way, the un-orchestrator is able to implement the behavior required by the 
`vlan` endpoint definition.

### Endpoint type: `gre-tunnel`
 
It represents an endpoint that is actually a GRE tunnel; hence, the UN guarantees 
that only the traffic encapsulated in a specific GRE tunnel enters from this 
endpoint, and that all the traffic  exiting from such an endpoint will be encapsulated 
in such a GRE tunnel. 
For instance, the `gre-tunnel` endpoint can be used to connect together pieces of the same service 
deployed  on different Universal Nodes.

The `gre-tunnel` endpoint is defined as follows:
	
	{
		"id": "00000002",
		"name": "egress",
		"type": "gre-tunnel",
		"gre-tunnel": {
			"local-ip": "10.0.0.1",
			"remote-ip": "10.0.0.2",
			"gre-key" : "1"
		}
	}
	
In this example, only the traffic belonging to the following GRE tunnel is enabled 
to enter through such an end point: `10.0.0.1` as a source IP address, `10.0.0.2` as a 
destination IP address, `1` as `gre-key`. Similarly, all the traffic that will exit from 
such an end point will be encapsulated into the GRE tunnel defined with the parameters 
listed before. Note that all the operations needed to reproduce this behavior (e.g., create 
the GRE tunnel, encpusulate the traffic in the GRE tunnel) are transparently implemented 
by the un-orchestrator.	

A complete example that shows the usage of the `gre-tunnel` endpoint is the following, in which 
the endpoint is associated with the following GRE tunnel: source IP `10.0.0.1`, destination IP `10.0.0.2"`, 
GRE key `1`. The traffic coming from this endpoint is provided to the `firewall` VNF, and then 
leaves the UN again through the `gre-tunnel` endpoint. 
 
 	{
		"forwarding-graph": 
		{
			"id": "00000001",
			"name": "Only network graph",
			"VNFs": [
		  	{
		    	"id": "00000001",
		    	"name": "firewall",
        		"ports": [
          		{
            		"id": "inout:0",
            		"name": "data-port"
          		},
          		{
            		"id": "inout:1",
            		"name": "data-port"
          		}
        		]
		  	}
		  	],
			"end-points": [
			{
		    	"id": "00000001",
		    	"name": "egress",
		    	"type": "gre-tunnel",
		    	"gre-tunnel": {
		      		"local-ip": "10.0.0.1",
		      		"remote-ip": "10.0.0.2",
		      		"gre-key" : "1"
		    	}
		  	}
		  	]
		  	"big-switch": {
		 		"flow-rules": [
		   	 	{
		      		"id": "00000001",
		      		"priority": 1,
		      		"match": {
		        		"port_in": "endpoint:00000001"
		      		},
		      		"actions": [
		        	{
		          		"output_to_port": "vnf:00000001:inout:0"
		        	}
		      		]
		    	},
		    	{
		      		"id": "00000002",
		      		"priority": 1,
		      		"match": {
		        		"port_in": "vnf:00000001:inout:1"
		      		},
		      		"actions": [
		        	{
		          		"output_to_port": "endpoint:00000001"
		        	}
		      		]
		    	}
		  	]
			}
	  	}
	}
	
### Endpoint type: `internal`
 
It is possible to connect multiple graphs together through the `internal` endpoint; particularly, 
the `internal-group` is used to identify all those graphs that must be connected inside the universal node.

The `internal` endpoint is defined as follows:
	
	{
		"id": "00000002",
		"name": "egress",
		"type": "internal",
		"internal":
		{
			"internal-group": "25"
		}
	}

As an example, the two following graphs define an `internal` endpoint belonging to
the `internal-group` `25`, and then they will be connected together. In particular,
the first graph provides all the traffic coming from the `internal` endpoint to a VNF;
then, packets coming from the VNF is sent again through the `internal` endpoint. 
Similarly, in the second graph traffic coming from a `gre-tunnel` endpoint is 
provided to the `internal` endpoint and vice versa.

	{
	  "forwarding-graph": {
	    "id": "myGraph",
	    "name": "Forwarding graph",
	    "end-points": [
	      {
		"id": "00000004",
		"name": "ingress",
		"type": "internal",
		"internal":
		{
			"internal-group" : "25"
		}
	      }
	    ],
	    "VNFs": [
	      {
		"vnf_template": "client.json",
		"id": "00000001",
		"name": "example",
		"ports": [
		  {
		    "id": "inout:0",
		    "name": "data-port"
		  }        
		]
	      }
	    ],
	    "big-switch": {
	      "flow-rules": [
		{
		  "id": "000000001",
		  "priority": 1,
		  "match": {
		    "port_in": "endpoint:00000004"
		  },
		  "actions": [
		    {
		      "output_to_port": "vnf:00000001:inout:0"
		    }
		  ]
		},
		{
		  "id": "000000002",
		  "priority": 1,
		  "match": {
		    "port_in": "vnf:00000001:inout:0"
		  },
		  "actions": [
		    {
		      "output_to_port": "endpoint:00000004"
		    }
		  ]
		}
	      ]
	    }
	  }
	}

	{
	  "forwarding-graph": {
	    "id": "00000002",
	    "name": "Forwarding graph",
	    "end-points": [
	      {
		"id": "00000005",
		"name": "egress",
		"type": "gre-tunnel",
		"gre-tunnel": {
		  "local-ip": "10.0.0.1",
		  "remote-ip": "10.0.0.2",
		  "gre-key" : "1"
		}
	      }
	    ],
	    "big-switch": {
	      "flow-rules": [
		{
		  "id": "000000001",
		  "priority": 1,
		  "match": {
		    "port_in": "endpoint:00000004"
		  },
		  "actions": [
		    {
		      "output_to_port": "endpoint:00000005"
		    }
		  ]
		},
		{
		  "id": "000000002",
		  "priority": 1,
		  "match": {
		    "port_in": "endpoint:00000005"
		  },
		  "actions": [
		    {
		      "output_to_port": "endpoint:00000004"
		    }
		  ]
		}
	      ]
	    }
	  }
	}
	
#### How this endpoint is implemented by the un-orchestrator

In the UN, each different `internal-group` is implemented with a graph (and then an LSI) that is connected to
the `LSI-0` with a number of links that is equal to twice the number of times the `internal` endpoint is used
by graphs. This graph is not connected to any VNF; moreover, unlike standardard graphs defined through the NF-FG, 
it implements the traditional L2 forwarding, hence it forwards packets based on the destianation MAC address.

## Configuration

A simple configuration mechanism is supported by the NF-FG formalism. In particular, it is possibile to:

  * assign the MAC address, IPv4 address and netmask to a specific VNF interface;
  * set environment variable to the VNF;
  * create ports of the VNF connected to the Internet through the UN control interface, and not through the NF-FG itself. 
    This also requires the creation of a TCP port forwarding between a TCP port in the host (UN) and in the VNF.
    
**WARNING**: all the elements starting with the string `unify` only work with Docker containers, and requires that a 
specific compilation flag for the un-orchestrator is enabled.
    
### Configuring the VNF interface

This is possibile by using the elements `mac` and `unify-ip` within the description of a VNF port, as show in the 
following example:

	"VNFs": [
	{
		"id": "00000001",
	   	"name": "firewall",
    	"ports": [
    	{
       		"id": "inout:0",
       		"name": "data-port",
       		"mac": "aa:bb:cc:dd:ee:ff",
       		"unify-ip": "192.168.0.1/24"
    	},
    	{
    		"id": "inout:1",
    		"name": "data-port",
    		"mac": "11:22:33:44:55:66",
       		"unify-ip": "10.0.0.1/24"
   		}
		]
  	}
  	]
  
Given this NF-FG, the un-orchestrator properly configures the VNF ports as specified by the graph itself.

### Setting environment variables to the VNF

This is possibile by using the element `unify-env-variables` within the description of a VNF. An example of usage 
of such a feature is the following:

	"VNFs": [
	{
		"id": "00000001",
	   	"name": "firewall",
	   	"unify-env-variables": [
	   	{
			"variable": "logginglevel=release"
	   	},
	   	{
			"variable": "block=tcp"
	   	}
	   	],
    	"ports": [
    	{
       		"id": "inout:0",
       		"name": "data-port"
    	},
    	{
    		"id": "inout:1",
    		"name": "data-port"
   		}
		]
  	}
  	]
  	
This examples sets the follwoing environment variables to the `firewall` VNF: 

  * `logginglevel` to the value `release`
  * `block` to `tcp`
  
It is worth noting that the un-orchestrator ignores the meaning of the environment variables; it
just set the specified variables into the VNF, without knowing their meaning and utility.

### Creating further VNF ports not connected to the NF-FG

It is possibile to create a further ports of VNFs, which are not connected to the NF-FG, but that will be connected 
by the un-orchestrator to the default switch created by the execution environment (e.g., `Docker0` in case 
of Docker containers). This operation requires to create a binding between a TCP port inside the VNF and 
a TCP port in the host, as shown in the following example:

	"VNFs": [
	{
		"id": "00000001",
	   	"name": "firewall",
	   	"unify-control": [
	   	{
			"host-tcp-port": 2000,
			"vnf-tcp-port":	80
	   	}
	   	],
    	"ports": [
    	{
       		"id": "inout:0",
       		"name": "data-port"
    	},
    	{
    		"id": "inout:1",
    		"name": "data-port"
   		}
		]
  	}
  	]

When the un-orchestrator receives an NF-FG with this information, it creates a third port connected to, 
e.g., `Docker0` in case of Docker containers, and sets up a port forwarding so that, all the traffic that 
arrives on the UN (by means of the control port) and directed to the TCP port 2000, is provided to the 
VNF on its own TCP port 80. 
Note that multiple port forwardings may be set up for a VNF; however, a single port connected to, e.g., 
`Docker0`, is created, regardless of the number of port forwardings required.
