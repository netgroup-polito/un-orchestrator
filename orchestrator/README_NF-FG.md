# NF-FG examples defined in WP5
This section provides some examples of NF-FGs that can be deployed on the Unviersal
Node through the un-orchestrator.

Some examples are also available in the `config` folder.

## Example 1

This example is very simple: configures a graph that receives all the traffic
from interface `eth0` and sends it to interface `eth1`, without traversing any
VNF.

    {
        "flow-graph":
        {
			"flow-rules": [
			{
				"id": "00000001",
				"match":
				{
					"port" : "eth0"
				},
                "action":
				{
					"port": "eth1"
				}
			}
			]
		}
	}


## Example 2

This example is more complex, and it includes a network function called "firewall".
Packets coming from the interface `eth0` are sent to the first port of the network
function (`firewall:1`), while packets coming from the second port of the network
function (`firewall:2`) are sent on the network interface `eth1`.

    {
        "flow-graph":
        {
            "VNFs": [
		    {
				"id": "firewall"
			}
		    ],
		    "flow-rules": [
		    {
			    "id": "00000001",
    			"match":
				{
					"port" : "eth0"
				},
				"action":
				{
					"VNF_id": "firewall:1"
				}
			},
			{
				"id": "00000002",
				"match":
				{
					"VNF_id" : "firewall:2"
				},
				"action":
				{
					"port": "eth1"
				}
			}
		    ]
    	}
    }


## Example 3

In this example, traffic coming from `eth0` is forwarded to the firewall through the port
`firewall:1`. Then, traffic coming from the firewall (`firewall:2`) is split based on the destination
TCP port. Packets directed to the TCP port 80 is provided to the web cache then to the NAT,
while all the other traffic is directly provided to the NAT. Finally, packets from `NAT:2` leaves the
graph through the port `eth2`.

This graph can be graphically represented as follows:

    eth0 -> firewall -> if (tcp_dst == 80) -> web cache  -> nat  -> eth1
                        else \--------------------------/

Json description of the graph:

    {
    	"flow-graph":
	    {
		    "VNFs": [
			{
				"id": "firewall"
			},
			{
				"id": "NAT"
			},
			{
				"id": "web-cache"
			}
	    	],
	    	"flow-rules": [
			{
				"id": "00000001",
				"match":
				{
					"port" : "eth0"
				},
				"action":
				{
					"VNF_id": "firewall:1"
				}
			},
			{
				"id": "00000002",
				"priority" : "10",
				"match":
				{
					"VNF_id" : "firewall:2",
					"tcp_dst" : "80"
				},
				"action":
				{
					"VNF_id": "web-cache:1"
				}
			},
			{
				"id": "00000003",
				"priority" : "1",
				"match":
				{
					"VNF_id" : "firewall:2"
				},
				"action":
				{
					"VNF_id": "NAT:1"
				}
			},
			{
				"id": "00000004",
				"match":
				{
					"VNF_id" : "web-cache:2"
				},
				"action":
				{
					"VNF_id": "NAT:1"
				}
			},
			{
				"id": "00000005",
				"match":
				{
					"VNF_id" : "NAT:2"
				},
				"action":
				{
					"port": "eth1"
				}
			}
    		]
	    }
    }


## Example 4

It is possible to connect multiple graphs together by using the so called "endpoints".
An endpoint is always in the form "graph_id:endpoint_id_in_the_graph", where "graph_id"
is the graph that defines the endpoint.
A graph that want to use and endpoint defined by another graph, can do it only if that
endpoint has been specified by that graph.

As an example, the following command defines an endpoint "myGraph:1", while the second
command uses that endpoint.

    {
        "flow-graph":
        {
            "VNFs":[
            {
                "id":"bridge"
            }
            ],
            "flow-rules":[
            {
                "id":"00000001",
                "match":
                {
                    "endpoint_id":"myGraph:1"
                },
                "action":
                {
                    "VNF_id":"bridge:1"
                }
            },
            {
                "id":"00000002",
                "match":
                {
                    "VNF_id":"bridge:2"
                },
                "action":
                {
                    "port":"eth1"
                }
            }
            ]
       }
    }

    {
       "flow-graph":
        {
            "VNFs":[
            {
                "id":"bridge"
            }
            ],
            "flow-rules":[
            {
                "id":"00000001",
                "priority":"25",
                "match":
                {
                    "port":"eth0",
                    "eth_src":"aa:aa:aa:aa:aa:aa"
                },
                "action":
                {
                    "endpoint_id":"myGraph:1"
                }
            },
            {
                "id":"00000002",
                "match":
                {
                    "port":"eth0"
                },
                "action":
                {
                    "port":"eth1"
                }
            }
            ]
        }
    }

  Since the endpoint "myGraph:1" is defined in a match of the graph "myGraph",
  other graphs can use it only in an action.
  On the other hand, if an endpoint is defined in an action, other graphs can
  use it in a match.

## Supported matches

Within the `match` element of the NF-FG description, the following fields are allowed
(all the values must be specified as strings):

	"port"         //only if "VNF_id" and "endpoint_id" are not specified
	"VNF_id"       //only if "port" and "endpoint_id" are not specified
	"endpoint_id"  //only if "port" and "VNF_id" are not specified
	"eth_src"
	"eth_src_mask"
	"eth_dst"
	"eth_dst_mask"
	"ethertype"
	"vlan_id"       //can be a number, "ANY", or "NO_VLAN"
	"vlan_pcp"
	"ip_dscp"
	"ip_ecn"
	"ip_proto"
	"ipv4_src"
	"ipv4_src_mask"
	"ipv4_dst"
	"ipv4_dst_mask"
	"tcp_src"
	"tcp_dst"
	"udp_src"
	"udp_dst"
	"sctp_src"
	"sctp_dst"
	"icmpv4_type"
	"icmpv4_code"
	"arp_opcode"
	"arp_spa"
	"arp_spa_mask"
	"arp_tpa"
	"arp_tpa_mask"
	"arp_sha"
	"arp_tha"
	"ipv6_src"
	"ipv6_src_mask"
	"ipv6_dst"
	"ipv6_dst_mask"
	"ipv6_flabel"
	"ipv6_nd_target"
	"ipv6_nd_sll"
	"ipv6_nd_tll"
	"icmpv6_type"
	"icmpv6_code"
	"mpls_label"
	"mpls_tc"

## Supported actions

Within the `action` element of the NF-FG description, one and only one of the
following fields **MUST** be specified:

	"port"         //only if "VNF_id" and "endpoint_id" are not specified
	"VNF_id"       //only if "port" and "endpoint_id" are not specified
	"endpoint_id"  //only if "port" and "VNF_id" are not specified

The previous fields indicates an output port through which packets can be sent.
Other actions can be specified together with the previous ones:

*  *vlan push*, which adds a specific vlan label to the packet;
*  *vlan pop*, which removes the more external vlan label to the packet.

The syntax to be used for these operations is the following:

	"action":
	{
        	"vlan":
		{
			"operation":"push",
			"vlan_id":"25"
		}
    	}


	"action":
	{
		"vlan":
		{
			"operation":"pop"
		}
	}

As an example, the following NF-FG tags all the packets coming from interface `eth0` and forwards them on interface `eth1`.

    {
        "flow-graph":
        {
			"flow-rules": [
			{
				"id": "00000001",
				"match":
				{
					"port" : "eth0"
				},
                "action":
				{
					"port": "eth1",
					"vlan":
					{
						"operation":"push",
						"vlan_id":"25"
					}
				}
			}
			]
		}
	}
