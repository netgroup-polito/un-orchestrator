#ifndef OVS_COMMANDS_H_
#define OVS_COMMANDS_H_ 1

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include <json_spirit/json_spirit.h>
#include <json_spirit/value.h>
#include <json_spirit/writer.h>

#include "../../../../utils/sockutils.h"

#include <iostream>
#include <fstream>

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <locale>
#include <sstream>
#include <vector>
#include "../../createLSIin.h"
#include "../../createLSIout.h"
#include "../../addNFports_in.h"
#include "../../addNFports_out.h"
#include "../../addVirtualLink_in.h"
#include "../../addVirtualLink_out.h"
#include "../../checkPhysicalPorts_in.h"
#include "../../destroyNFports_in.h"

#include "inttypes.h"
#include <string.h>

#define INTERRUPTED_BY_SIGNAL (errno == EINTR || errno == ECHILD)

using namespace std;
using namespace json_spirit;

class commands
{
private:
	/*
	*	@brief: IPsec certificate
	*/
	string ipsec_certificate;

	/*
	*	@brief: Map that, given a port name used by the VNF, provides the name of such a port on the switch.
	*			It is needed in case of VETH ports, in which the name of the end given to the VNF is different with
	*			respect to the name of the end attached to the switch
	*/
	map<string,string> peersNames;
public:
	commands();

	~commands();

	int cmd_connect();

	int cmd_disconnect(int s);

	/**
	*	Example of command to create a new LSI
	*
		{
			"id" : 0,
			"method" : "transact",
			"params" : [
				"Open_vSwitch",
				{
				    "op" : "insert",
				    "row" : {
				        "connection_mode" : "out-of-band",
				        "is_connected" : true,
				        "local_ip" : "127.0.0.1",
				        "target" : "tcp:127.0.0.1:6653"
				    },
				    "table" : "Controller",
				    "uuid-name" : "ctrl1"
				},
				{
				    "op" : "insert",
				    "row" : {
				        "controller" : [
				            "set",
				            [
				                [
				                    "named-uuid",
				                    "ctrl1"
				                ]
				            ]
				        ],
				        "name" : "Bridge1",
				        "other_config" : [
				            "map",
				            [
				                [
				                    "disable-in-band",
				                    "true"
				                ]
				            ]
				        ],
				        "ports" : [
				            "set",
				            [
				            ]
				        ],
				        "protocols" : "OpenFlow12"
				    },
				    "table" : "Bridge",
				    "uuid-name" : "Bridge1"
				},
				{
				    "mutations" : [
				        [
				            "bridges",
				            "insert",
				            [
				                "set",
				                [
				                    [
				                        "named-uuid",
				                        "Bridge1"
				                    ]
				                ]
				            ]
				        ]
				    ],
				    "op" : "mutate",
				    "table" : "Open_vSwitch",
				    "where" : [
				    ]
				}
			]
		}
	*/
	CreateLsiOut* cmd_editconfig_lsi (CreateLsiIn cli, int s);

	/**
	*	Example of command to create a new SYSTEM PORT
	*
		{
			"id" : 1,
			"method" : "transact",
			"params" : [
				"Open_vSwitch",
				{
				    "op" : "insert",
				    "row" : {
				        "admin_state" : "up",
				        "link_state" : "up",
				        "name" : "eth3",
				        "ofport" : 1,
				        "ofport_request" : 1
				    },
				    "table" : "Interface",
				    "uuid-name" : "iface1"
				},
				{
				    "op" : "insert",
				    "row" : {
				        "interfaces" : [
				            "set",
				            [
				                [
				                    "named-uuid",
				                    "iface1"
				                ]
				            ]
				        ],
				        "name" : "eth3"
				    },
				    "table" : "Port",
				    "uuid-name" : "eth3"
				},
				{
				    "op" : "update",
				    "row" : {
				        "ports" : [
				            "set",
				            [
				                [
				                    "named-uuid",
				                    "eth3"
				                ]
				            ]
				        ]
				    },
				    "table" : "Bridge",
				    "where" : [
				        [
				            "_uuid",
				            "==",
				            [
				                "uuid",
				                "e5079fd5-adb5-4eea-8348-12c39a9e0352"
				            ]
				        ]
				    ]
				},
				{
				    "mutations" : [
				        [
				            "next_cfg",
				            "+=",
				            1
				        ]
				    ],
				    "op" : "mutate",
				    "table" : "Open_vSwitch",
				    "where" : [
				    ]
				}
			]
		}
	*/
	/**
	 * Add a port to the LSI identifed by dnumber. Returns the port name on as known on the switch.
	 */
	string add_port(string p, uint64_t dnumber, bool is_nf_port, int s, PortType port_type = UNDEFINED_PORT);

	/*
	*	Example of command to create a new GRE PORT
	*
		{
			"id" : 9,
			"method" : "transact",
			"params" : [
				"Open_vSwitch",
				{
				    "op" : "insert",
				    "row" : {
				        "admin_state" : "up",
				        "link_state" : "up",
				        "name" : "gre2",
				        "ofport" : 8,
				        "ofport_request" : 8,
				        "options" : [
				            "map",
				            [
				                [
				                    "remote_ip",
				                    "10.0.0.1"
				                ],
				                [
				                	"local_ip",
				                	"10.0.0.1"
				                ],
				                [
				                	"in_key",
				                	"1"
				                ],
				                [
				                	"out_key",
				                	"1"
				                ],
				                [
				                	"key",
				                	"1"
				                ]
				            ]
				        ],
				        "type" : "gre"
				    },
				    "table" : "Interface",
				    "uuid-name" : "iface8"
				},
				{
				    "op" : "insert",
				    "row" : {
				        "interfaces" : [
				            "set",
				            [
				                [
				                    "named-uuid",
				                    "iface8"
				                ]
				            ]
				        ],
				        "name" : "gre2"
				    },
				    "table" : "Port",
				    "uuid-name" : "gre2"
				},
				{
				    "op" : "update",
				    "row" : {
				        "ports" : [
				            "set",
				            [
				                [
				                    "uuid",
				                    "9ab63040-1a10-4dc3-9606-d78386773fff"
				                ],
				                [
				                    "uuid",
				                    "eacd9aa2-eb2a-47e6-888b-364c4fe2d4c3"
				                ],
				                [
				                    "uuid",
				                    "9b4efcb3-28ee-4e61-bea5-f7255a7e9171"
				                ],
				                [
				                    "named-uuid",
				                    "gre2"
				                ]
				            ]
				        ]
				    },
				    "table" : "Bridge",
				    "where" : [
				        [
				            "_uuid",
				            "==",
				            [
				                "uuid",
				                "3cb09eac-7f03-4099-98c5-168f83790eac"
				            ]
				        ]
				    ]
				},
				{
				    "mutations" : [
				        [
				            "next_cfg",
				            "+=",
				            1
				        ]
				    ],
				    "op" : "mutate",
				    "table" : "Open_vSwitch",
				    "where" : [
				    ]
				}
			]
		}
	*/
	void add_endpoint(uint64_t dpi, char local_ip[BUF_SIZE], char remote_ip[BUF_SIZE], char key[BUF_SIZE], char port_name[BUF_SIZE], char ifac[BUF_SIZE], int s, char is_safe[BUF_SIZE]);

	/**
	*	Example of command to create a new INTERNAL PORT
	*
		{
			"id" : 4,
			"method" : "transact",
			"params" : [
				"Open_vSwitch",
				{
				    "op" : "insert",
				    "row" : {
				    	"type" : "internal"
				        "admin_state" : "up",
				        "link_state" : "up",
				        "name" : "eth3",
				        "ofport" : 1,
				        "ofport_request" : 1
				    },
				    "table" : "Interface",
				    "uuid-name" : "iface1"
				},
				{
				    "op" : "insert",
				    "row" : {
				        "interfaces" : [
				            "set",
				            [
				                [
				                    "named-uuid",
				                    "iface1"
				                ]
				            ]
				        ],
				        "name" : "eth3"
				    },
				    "table" : "Port",
				    "uuid-name" : "eth3"
				},
				{
				    "op" : "update",
				    "row" : {
				        "ports" : [
				            "set",
				            [
				                [
				                    "named-uuid",
				                    "eth3"
				                ]
				            ]
				        ]
				    },
				    "table" : "Bridge",
				    "where" : [
				        [
				            "_uuid",
				            "==",
				            [
				                "uuid",
				                "e5079fd5-adb5-4eea-8348-12c39a9e0352"
				            ]
				        ]
				    ]
				},
				{
				    "mutations" : [
				        [
				            "next_cfg",
				            "+=",
				            1
				        ]
				    ],
				    "op" : "mutate",
				    "table" : "Open_vSwitch",
				    "where" : [
				    ]
				}
			]
		}
	*/
	AddNFportsOut *cmd_editconfig_NFPorts(AddNFportsIn anpi, int s);

	AddEndpointOut* cmd_editconfig_endpoint (AddEndpointIn aepi, int s);

	AddVirtualLinkOut *cmd_addVirtualLink(AddVirtualLinkIn avli, int s);

	/*
	*	Example of command to create a new PATCH PORT
	*
		{
			"id" : 9,
			"method" : "transact",
			"params" : [
				"Open_vSwitch",
				{
				    "op" : "insert",
				    "row" : {
				        "admin_state" : "up",
				        "link_state" : "up",
				        "name" : "vport2",
				        "ofport" : 8,
				        "ofport_request" : 8,
				        "options" : [
				            "map",
				            [
				                [
				                    "peer",
				                    "vport3"
				                ]
				            ]
				        ],
				        "type" : "patch"
				    },
				    "table" : "Interface",
				    "uuid-name" : "iface8"
				},
				{
				    "op" : "insert",
				    "row" : {
				        "interfaces" : [
				            "set",
				            [
				                [
				                    "named-uuid",
				                    "iface8"
				                ]
				            ]
				        ],
				        "name" : "vport2"
				    },
				    "table" : "Port",
				    "uuid-name" : "vport2"
				},
				{
				    "op" : "update",
				    "row" : {
				        "ports" : [
				            "set",
				            [
				                [
				                    "uuid",
				                    "9ab63040-1a10-4dc3-9606-d78386773fff"
				                ],
				                [
				                    "uuid",
				                    "eacd9aa2-eb2a-47e6-888b-364c4fe2d4c3"
				                ],
				                [
				                    "uuid",
				                    "9b4efcb3-28ee-4e61-bea5-f7255a7e9171"
				                ],
				                [
				                    "named-uuid",
				                    "vport2"
				                ]
				            ]
				        ]
				    },
				    "table" : "Bridge",
				    "where" : [
				        [
				            "_uuid",
				            "==",
				            [
				                "uuid",
				                "3cb09eac-7f03-4099-98c5-168f83790eac"
				            ]
				        ]
				    ]
				},
				{
				    "mutations" : [
				        [
				            "next_cfg",
				            "+=",
				            1
				        ]
				    ],
				    "op" : "mutate",
				    "table" : "Open_vSwitch",
				    "where" : [
				    ]
				}
			]
		}
	*/
	void cmd_add_virtual_link(string vrt, string trv, char ifac[BUF_SIZE], uint64_t dpi, int s);

	void cmd_destroyVirtualLink(DestroyVirtualLinkIn dvli, int s);

	/*
	*	Example of command to destroy a BRIDGE
	*
		{
			"id" : 16,
			"method" : "transact",
			"params" : [
				"Open_vSwitch",
				{
				    "op" : "update",
				    "row" : {
				        "bridges" : [
				            "set",
				            [
				                [
				                    "uuid",
				                    "e5079fd5-adb5-4eea-8348-12c39a9e0352"
				                ]
				            ]
				        ]
				    },
				    "table" : "Open_vSwitch",
				    "where" : [
				    ]
				},
				{
				    "mutations" : [
				        [
				            "next_cfg",
				            "+=",
				            1
				        ]
				    ],
				    "op" : "mutate",
				    "table" : "Open_vSwitch",
				    "where" : [
				    ]
				}
			]
		}
	*/
	void cmd_editconfig_lsi_delete(uint64_t dpid, int s);

	/*
	*	Example of command to destroy a PORT
	*
		{
			"id" : 16,
			"method" : "transact",
			"params" : [
				"Open_vSwitch",
				{
				    "op" : "update",
				    "row" : {
				        "ports" : [
				            "set",
				            [
				                [
				                    "uuid",
				                    "e5079fd5-adb5-4eea-8348-12c39a9e0352"
				                ]
				            ]
				        ]
				    },
				    "table" : "Bridge",
				    "where" : [
				    ]
				},
				{
				    "mutations" : [
				        [
				            "next_cfg",
				            "+=",
				            1
				        ]
				    ],
				    "op" : "mutate",
				    "table" : "Open_vSwitch",
				    "where" : [
				    ]
				}
			]
		}
	*/
	void cmd_editconfig_NFPorts_delete(DestroyNFportsIn dnpi, int s);

	void cmd_editconfig_endpoint_delete(DestroyEndpointIn depi, int s);

	void cmd_delete_virtual_link(uint64_t dpid, uint64_t id, int s);

};

class commandsException: public SwitchManagerException
{
public:
	virtual const char* what() const throw()
	{
		return "commandsException";
	}
};

#endif //OVS_COMMANDS_H_
