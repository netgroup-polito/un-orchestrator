CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(node-orchestrator)

ADD_DEFINITIONS(-Wall -Werror -Wno-write-strings -Werror -Wno-pointer-sign -Werror -Wno-return-type -Werror -Wno-format-security -Werror -unused-function)

# Rather complicated CMake code for setting the logging level
ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_DEBUG_INFO)
# End of the rather complicated CMake code for setting the logging level

# Rather complicated CMake code for selecting the virtual switch implementation
ADD_DEFINITIONS(-DVSWITCH_IMPLEMENTATION_OVSDB)

#################################################################################################################

# Set source files
SET(SOURCES
	node_orchestrator.cc

	node_resource_manager/graph_manager/graph_manager.h
	node_resource_manager/graph_manager/graph_manager.cc
	node_resource_manager/graph_manager/graph_info.h
	node_resource_manager/graph_manager/graph_info.cc
	node_resource_manager/graph_manager/graph_translator.h
	node_resource_manager/graph_manager/graph_translator.cc
	node_resource_manager/graph_manager/rule_removed_info.h
	node_resource_manager/graph_manager/lsi.h
	node_resource_manager/graph_manager/lsi.cc
	node_resource_manager/graph_manager/virtual_link.h
	node_resource_manager/graph_manager/virtual_link.cc

	node_resource_manager/graph/match.h
	node_resource_manager/graph/match.cc
	node_resource_manager/graph/generic_action.h
	node_resource_manager/graph/generic_action.cc
	node_resource_manager/graph/vlan_action.h
	node_resource_manager/graph/vlan_action.cc

	node_resource_manager/graph/graph-parser/graph_parser.h
	node_resource_manager/graph/graph-parser/graph_parser.cc
	node_resource_manager/graph/graph-parser/match_parser.h
	node_resource_manager/graph/graph-parser/match_parser.cc

	node_resource_manager/graph/low_level_graph/output_action.h
	node_resource_manager/graph/low_level_graph/output_action.cc
	node_resource_manager/graph/low_level_graph/graph.h
	node_resource_manager/graph/low_level_graph/graph.cc
	node_resource_manager/graph/low_level_graph/low_level_match.h
	node_resource_manager/graph/low_level_graph/low_level_match.cc
	node_resource_manager/graph/low_level_graph/rule.h
	node_resource_manager/graph/low_level_graph/rule.cc

	node_resource_manager/graph/high_level_graph/high_level_output_action.h
	node_resource_manager/graph/high_level_graph/high_level_output_action.cc
	node_resource_manager/graph/high_level_graph/high_level_output_action_nf.h
	node_resource_manager/graph/high_level_graph/high_level_output_action_nf.cc
	node_resource_manager/graph/high_level_graph/high_level_output_action_port.h
	node_resource_manager/graph/high_level_graph/high_level_output_action_port.cc
	node_resource_manager/graph/high_level_graph/high_level_output_action_endpoint_gre.h
	node_resource_manager/graph/high_level_graph/high_level_output_action_endpoint_gre.cc
	node_resource_manager/graph/high_level_graph/high_level_output_action_endpoint_internal.h
	node_resource_manager/graph/high_level_graph/high_level_output_action_endpoint_internal.cc
	node_resource_manager/graph/high_level_graph/high_level_rule.h
	node_resource_manager/graph/high_level_graph/high_level_rule.cc
	node_resource_manager/graph/high_level_graph/high_level_graph_endpoint_interface.cc
	node_resource_manager/graph/high_level_graph/high_level_graph_endpoint_interface.h
	node_resource_manager/graph/high_level_graph/high_level_graph_endpoint_gre.cc
	node_resource_manager/graph/high_level_graph/high_level_graph_endpoint_gre.h
	node_resource_manager/graph/high_level_graph/high_level_graph_endpoint_internal.cc
	node_resource_manager/graph/high_level_graph/high_level_graph_endpoint_internal.h
	node_resource_manager/graph/high_level_graph/high_level_graph_endpoint_vlan.cc
	node_resource_manager/graph/high_level_graph/high_level_graph_endpoint_vlan.h
	node_resource_manager/graph/high_level_graph/high_level_graph_vnf.cc
	node_resource_manager/graph/high_level_graph/high_level_graph_vnf.h
	node_resource_manager/graph/high_level_graph/high_level_graph.h
	node_resource_manager/graph/high_level_graph/high_level_graph.cc
	node_resource_manager/graph/high_level_graph/high_level_match.h
	node_resource_manager/graph/high_level_graph/high_level_match.cc

	network_controller/switch_manager/switch_manager.h
	network_controller/switch_manager/addNFports_in.h
	network_controller/switch_manager/addNFports_out.h
	network_controller/switch_manager/addVirtualLink_in.h
	network_controller/switch_manager/addVirtualLink_out.h
	network_controller/switch_manager/createLSIout.h
	network_controller/switch_manager/destroyNFports_in.h
	network_controller/switch_manager/destroyVirtualLink_in.h
	network_controller/switch_manager/checkPhysicalPorts_in.h

	network_controller/openflow_controller/controller.h
	network_controller/openflow_controller/controller.cc

	compute_controller/nfs_manager.h
	compute_controller/nfs_manager.cc
	compute_controller/compute_controller.h
	compute_controller/compute_controller.cc
	compute_controller/nf.h
	compute_controller/nf.cc
	compute_controller/nf_type.h
	compute_controller/description.h
	compute_controller/description.cc
	compute_controller/startNF_in.h
	compute_controller/stopNF_in.h

	node_resource_manager/rest_server/rest_server.h
	node_resource_manager/rest_server/rest_server.cc

	node_resource_manager/database_manager/SQLite/SQLiteManager.h
	node_resource_manager/database_manager/SQLite/SQLiteManager.cc
	node_resource_manager/database_manager/SQLite/SQLiteManager_constants.h
	node_resource_manager/database_manager/SQLite/INIReader.cpp
	node_resource_manager/database_manager/SQLite/ini.c

	node_resource_manager/security_manager/security_manager.cc
	node_resource_manager/security_manager/security_manager.h

	utils/logger.h
	utils/logger.c
	utils/constants.h
	utils/sockutils.h
	utils/sockutils.c
)

SET(SOURCES1
    ${SOURCES}
	network_controller/switch_manager/plugins/ovs-ovsdb/commands.h
	network_controller/switch_manager/plugins/ovs-ovsdb/commands.cc
	network_controller/switch_manager/plugins/ovs-ovsdb/ovsdb_constants.h
	network_controller/switch_manager/plugins/ovs-ovsdb/ovsdb_manager.h
    network_controller/switch_manager/plugins/ovs-ovsdb/ovsdb_manager.cc
)

INCLUDE_DIRECTORIES (
	/usr/local/include
	/usr/local/lib
	/usr/include/boost
	/usr/include/libxml2
)

# Create the executable
ADD_EXECUTABLE(
	node-orchestrator
	${SOURCES1}
)

LINK_DIRECTORIES(
	/usr/lib/
	/usr/local/lib/
	/usr/x86_64-linux-gnu/
	/usr/lib/x86_64-linux-gnu/
	/usr/lib/python2.7/config-x86_64-linux-gnu/
)

TARGET_LINK_LIBRARIES( node-orchestrator
	libpthread.so
	libmicrohttpd.so
	libboost_system.so
	libxml2.so
	libsqlite3.so
	libcrypto.so
	libjson_spirit.so
	librofl_common.so
	-lrt
)
