# REST API

The un-orchestrator can either accept a NF-FG from a file, or from its REST interface,
that thanks to a small HTTP server embedded in the UN. The main REST commands to be used
to interact with the un-orchestrator (e.g., deploy a new graph, update an existing graph,
etc.) are detailed in this document.

## REST commands accepted by the un-orchestrator

### NF-FG API
Deploy an NF-FG called ``myGraph'', according to the graph formalism described in [README_NF-FG.md](README_NF-FG.md):

    PUT /NF-FG/myGraph HTTP/1.1
    Content-Type : application/json

    {
		"forwarding-graph": {
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
			  }
			],
			"big-switch": {
			  "flow-rules": [
				{
				  "id": "000000001",
				  "priority": 1,
				  "match": {
					"port_in": "endpoint:00000001"
				  },
				  "actions": [
					{
					  "output_to_port": "vnf:00000001:inout:0"
					}
				  ]
				}
			  ]
			}
		}
	}

The same message used to create a new graph can be used to update a creaph, i.e., to

 * add/remove flow-rules from the big-switch
 * add/remove virtual network functions
 * add/remove ports from virtual network functions already deployed
 * add/remove endpoints (interface, gre-tunnel, vlan, internal)

To update a graph, you have just to send the new version of the graph at the same URL of
the graph to be updated (e.g., /NF-FG/myGraph); the un-orchestrator will then calculate the
difference between the new version and that already deployed, and will do all the proper
operations to update the graph as required.

Retrieve the description of the graph with name "myGraph":

	GET /NF-FG/myGraph HTTP/1.1

Retrieve the information of the status of the graph with name "myGraph":

	GET /NF-FG/status/myGraph HTTP/1.1

Delete the graph with name "myGraph"

	DELETE /NF-FG/myGraph HTTP/1.1

Retrieve the description of all the graphs

	GET /NF-FG HTTP/1.1

### Authentication API

The un-orchestrator supports user authentication, which has to be enabled through the configuration file of the module.
In case this feature is turned on, all the interactions with the UN must start with an authentication message, which looks like the following:

    POST /login	HTTP/1.1
    Content-Type : application/json

    {
        "username":"admin", 
        "password":"admin"
    }
    
If the authentication is successful, this method will return a token in the response.

The returned token has to be used by creating an additional `X-Auth-Token` header in all the requests you send to the UN.
In this way the UN will know the identity of the user and it will be able to check whether the user has the right to perform the requested operation or not.

Users and permissions are stored in a local SQLite database.

### Groups API

Create a new users group called 'myGroup'

	PUT /groups/myGroup HTTP/1.1
	
Delete an existing group called 'myGroup'

	DELETE /groups/myGroup HTTP/1.1
	
Retrieve the information about all the groups

	GET /groups HTTP/1.1

### Users API

Create a new user called 'myUser', specifing the group the user belongs to

    POST /users/myUser	HTTP/1.1
    Content-Type : application/json

    {
        "password":"sample_pwd",
        "group":"sample_group"
    }
    
Retrieve the information the users called 'myUser'

	GET /users/myUser HTTP/1.1
	
Delete the users called 'myUser'

	DELETE /users/myUser HTTP/1.1
	
Retrieve the information about all the users

	GET /users HTTP/1.1
	
	
## How to send commands to the un-orchestrator

In order to interact with the un-orchestrator through its REST API, you can use
your favorite REST tool (e.g., some nice plugins for Mozilla Firefox). Just in
also use the CURL command line tool, such as in the following example (where the
NF-FG to be instantiated is stored in the file 'myGraph.json'):

	$ curl -i -H "Content-Type: application/json" -d "@myGraph.json" \
		-X PUT  http://un-orchestrator-address:port/NF-FG/myGraph
		

