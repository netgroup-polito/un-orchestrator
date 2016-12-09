# How to launch a Virtual Network Function (VNF) on the Universal Node

This document details how to deploy and run a Network Function (NF) on the Universal Node (UN). This requires the execution of the following main steps (detailed in the remainder of the document):
*	create the desired NF image and the associated template;
*	upload the NF and the template in the datastore; 
*	send to the un-orchestrator a Network Functions-Forwarding Graph (NF-FG) including the NF that has to be instantiated.

### Create the NF image
The Universal Node currently supports four types of NFs: NFs executed as Docker containers, NFs executed inside KVM-based virtual machines, NFs based on the DPDK library (i.e., DPDK processes), and the native network functions.
In order to create your own NF image, please check individual README's in each sub-package.
You can find many examples of templates in the [datastore](../datastore) folder.

### Upload the NF and the template in the datastore
Once the NF image and the associated template are created, they must be uploaded in the datastore.
To install the datastore, please check the README file in [datastore](../datastore).
At the same link, you can also find a description of the API exported by such a module, and that can be used to upload the NF image and its template.

### Provide the graph description to the un-orchestrator
In order to deploy your NF on the UN, you must provide to the un-orchestration a NF-FG including such a NF (to compile and then execute the un-orchestrator, please check the files [orchestrator/README_COMPILE.md](../orchestrator/README_COMPILE.md) and [orchestrator/README_RUN.md](../orchestrator/README_RUN.md)).

The un-orchestrator supports two NF-FG versions:
  * the JSON-based format, which is supported natively (more information is available in [orchestrator/README_NF-FG.md](../orchestrator/README_NF-FG.md) and in [orchestrator/README_RESTAPI.md](../orchestrator/README_RESTAPI.md));
  * the  XML-based format defined in WP3 that includes both top-down
    communication (for the actual forwarding graph) and bottom-up primitives
    (for resources and capabilities). This version of the NF-FG requires the
    usage of the [`virtualizer`](../virtualizer/README.md).

## An example

This section shows an example in which a NF called `dummy` and executed as a Docker container is deployed and then run on the Universal Node.
This NF is deployed as part of the service shown in the picture:

![service-graph](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/service-graph.png)

To create the NF image and store it in the local file system of the Universal Node, execute the following command in the folder containing the Docker file describing the NF:

	sudo docker build --tag="dummy" .
	sudo docker save dummy

Then, upload the NF and its template in the datastore.

At this point, prepare a NF-FG and pass it to the un-orchestator, which will take care of executing all the operations required to implement the graph. The graph shown in the picture above can be described in the native JSON syntax as follow:

	{
		"forwarding-graph": 
		{
			"id": "00000001",
			"name": "Forwarding graph",
			"VNFs": [
		  	{
		    	"id": "00000001",
		    	"name": "dummy",
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
		      		"interface": "eth0"
		    	}
		  	},
		  	{
		    	"id": "00000002",
		    	"name": "egress",
		    	"type": "interface",
		    	"interface": {
		      		"interface": "eth1"
		    	}
		  	}
			],
			"big-switch": {
		  		"flow-rules": [
		    	{
		      		"id": "000000001",
		      		"priority": 100,
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
		      		"id": "000000002",
		      		"priority": 100,
		      		"match": {
		        		"port_in": "vnf:00000001:inout:1"
		      		},
		      		"actions": [
		        	{
		          		"output_to_port": "endpoint:00000002"
		        	}
		      		]
		    	},
		    	{
		      		"id": "000000003",
		      		"priority": 100,
		      		"match": {
		        		"port_in": "endpoint:00000002"
		      		},
		      		"actions": [
		        	{
		        		"output_to_port": "vnf:00000001:inout:1"
		        	}
		      		]
		    	},
		    	{
		      		"id": "000000004",
		      		"priority": 100,
		      		"match": {
		        		"port_in": "vnf:00000001:inout:0"
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

This json can be stored in a file (e.g., `nffg.json`) and provided to the un-orchestrator either through the command line at the boot of the un-orchestrator, or through its REST API. In the latter case, the command to be used is the following:

	curl -i -H "Content-Type: application/json" -d "@nffg.json" \
		-X PUT  http://un-orchestrator-address:port/NF-FG/graphid

where the `graphid` is an alphanumeric string that will uniquely identify your graph in the un-orchestrator.

At this point the un-orchestrator
*	creates a new LSI through the *network controller*, inserts the proper Openflow rules in such an LSI in order to steer the traffic among the NFs of the graph, and inserts the proper Openflow rules in the LSI-0 (which is the only LSI connected to the physical interfaces) in order to inject the proper traffic in the graph, and properly handle the network packets exiting from such a graph;
*	starts the Docker image implementing the NF with name *dummy* through the *compute controller*.

The following picture shows how the NF-FG of the example is actually implemented on the UN; in particular, it depicts the connections among LSIs and the NF, and the rules in the flow tables of the involved LSIs.

![deployment](https://raw.githubusercontent.com/netgroup-polito/un-orchestrator/master/images/deployment.png)

To conclude, the deployment of a second graph will trigger the creation of a new LSI, again connected with the LSI-0; the LSI-0 will then be instructed to properly dispatch the traffic coming from the physical ports among the deployed NF-FGs, according the the NF-FGs themselves.
