# Virtualizer

The Virtualizer is an intermediate module sit between the un-orchestrator and the upper layers 
of the Unify architecture. It operates as follows:
  * receives commands from the upper layers of the Unify architecture based on the virtualizer 
    library defined by WP3;
  * converts those commands in the formalism natively supported by the un-orchestrator
    (described in [../orchestrator/README_NF-FG.md](../orchestrator/README_NF-FG.md));
  * sends the command to the un-orchestrator through the API described in [../orchestrator/README_RESTAPI.md](../orchestrator/README_RESTAPI.md)

This module is only required to integrate the un-orchestrator with the upper layers of the
Unify architecture, hence to enable the orchestrator itself to *use* the Network Functions - 
Forwarding Graph (NF-FG) defined in WP3, which is based on the concept of *virtualizer*.
It is instead not needed to use the un-orchestrator in the standalone mode; in the case, the native 
NF-FG can be used ([../orchestrator/README_NF-FG.md](../orchestrator/README_NF-FG.md)).


## Required libraries

In addition to the libraries already listed in the main [../README_COMPILE.md](../README_COMPILE.md),
some more components are required to run the virtualizer.

	; Retrieve the library used 
	; to handle the NF-FG in the format natively supported by the un-orchestrator.
	$ cd [un-orchestrator]
	$ git submodule update --init --recursive

	; Install other required libraries 
	$ sudo apt-get install python-pip
	$ sudo pip install gunicorn falcon cython requests

The virtualizer module relies on the virtualizer library (v5) that is not included in this repository but it can be found on the UNIFY repository. The virtualizer library's python files must then be put in the [virtualizer_library](virtualizer_library) directory
## How to configure the Virtualizer

The Virtualizer reads its configuration from the file [./config/configuration.ini](config/configuration.ini), 
which must be properly edited before starting the Virtualizer itself.

## How to run the Virtualizer

	$ gunicorn -b ip:port virtualizer:api

where 'ip' and 'port' must be set to the desired values.

Please, note that the Virtualizer requires the un-orchestrator and the 
name-resolver running in the server.

## Configuration file examples

Examples of configurations that can be sent to the Virtualizer are available in [./config/nffg_examples](nffg_examples).
In particular:
  * [./config/nffg_examples/simple_passthrough_nffg.xml](./config/nffg_examples/simple_passthrough_nffg.xml): 
    simple configuration that implements a simple passthrough function, i.e., traffic is 
    received from a first physical port and sent out from a second physical port, 
    after having been handled to the vSwitch;
  * [./config/nffg_examples/passthrough_with_vnf_nffg.xml](./config/nffg_examples/passthrough_with_vnf_nffg.xml): 
    configuration that includes a VNF. Traffic is received from a first physical 
    port, provided to a network function, and then sent out from a second physical 
    port;
  * [./config/nffg_examples/passthrough_with_vnf_nffg_and_match_and_action.xml](./config/passthrough_with_vnf_nffg_and_match_and_action.xml): 
    this configuration includes flows matching some protocol fields, and having 
    actions that manipulate protocol fields;
  * [./config/nffg_examples/nffg_delete_flow_vnf.xml](./config/nffg_examples/nffg_delete_flow_vnf.xml): 
    configuration that deletes some flows and a VNF instantiated on the Universal 
    Node.

## Rest API

The Virtualizer accept commands through its REST interface. The main REST commands 
to be used to interact with it are detailed in the following.

Get information about the Virtualizer

    GET / HTTP/1.1
    
Test the Virtualizer aliveness

    GET /ping HTTP/1.1

Retrieve the current configuration of the universal node

    POST /get-config HTTP/1.1

This command returns the current configuration in the format defined by the virtualzier library

Deploy a new configuration 

    POST /edit-config HTTP/1.1

The body of the message must contain the new configuration for the universal node 
espressed in the format defined by the Virtualizer library.

### Send commands to the Virtualzier
    
In order to interact with the Virtualizer throug its REST API, you can use your favorite REST tool (e.g., some nice 
plugins for Mozilla Firefox). Just in also use the CURL command line tool, such as in the following example 
(where the NF-FG to be instantiated is stored in the file 'myGraph.xml'):

$ curl -i -d "@myGraph.json" -X POST  http://virtualizer-address:port/edit-config
