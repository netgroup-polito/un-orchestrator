CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(node-orchestrator)

IF(CMAKE_COMPILER_IS_GNUCXX)
        ADD_DEFINITIONS(-Wall -Werror -Wno-write-strings)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)
IF(CMAKE_COMPILER_IS_GNUCC)
        ADD_DEFINITIONS(-Wall -Werror -Wno-write-strings)
ENDIF(CMAKE_COMPILER_IS_GNUCC)

OPTION(
	DETAILED_LOGS
	"Turn on to have a detailed logs"
	OFF
)

IF(DETAILED_LOGS)
	ADD_DEFINITIONS(-DDETAILED_LOGS)
ENDIF(DETAILED_LOGS)

# Rather complicated CMake code for selecting the virtual switch implementation
SET(VSWITCH_IMPLEMENTATION "OVS-OVSDB" CACHE STRING "vSwitch implementation: XDPD, OVS-DPDK, OVS-OVSDB, ERFS")
SET(VSWITCH_IMPLEMENTATION_VALUES "XDPD" "OVS-DPDK" "OVS-OVSDB" "ERFS") #Add here other implementations
SET_PROPERTY(CACHE VSWITCH_IMPLEMENTATION PROPERTY STRINGS ${VSWITCH_IMPLEMENTATION_VALUES})
LIST(FIND VSWITCH_IMPLEMENTATION_VALUES ${VSWITCH_IMPLEMENTATION} VSWITCH_IMPLEMENTATION_INDEX)

IF(${VSWITCH_IMPLEMENTATION_INDEX} EQUAL -1)
	message(FATAL_ERROR "Option ${VSWITCH_IMPLEMENTATION} not supported, valid entries are ${VSWITCH_IMPLEMENTATION_VALUES}")
ENDIF()
IF(${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 0)
	ADD_DEFINITIONS(-DVSWITCH_IMPLEMENTATION_XDPD)
ENDIF()
IF(${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 1)
	#OVS-DPDK uses the OVSDB plugin with some specific code needed by DPDK
	ADD_DEFINITIONS(-DVSWITCH_IMPLEMENTATION_OVSDB)
	ADD_DEFINITIONS(-DENABLE_OVSDB_DPDK)
ENDIF()
IF(${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 2)
	ADD_DEFINITIONS(-DVSWITCH_IMPLEMENTATION_OVSDB)
ENDIF()
IF(${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 3)
        ADD_DEFINITIONS(-DVSWITCH_IMPLEMENTATION_ERFS)
ENDIF()

OPTION(
	STARTVNF_SINGLE_THREAD
	"Turn on to start the VNF sequentially"
	OFF
)

IF(STARTVNF_SINGLE_THREAD)
	ADD_DEFINITIONS(-DSTARTVNF_SINGLE_THREAD)
ENDIF(STARTVNF_SINGLE_THREAD)

OPTION(
	ENABLE_UNIFY_PORTS_CONFIGURATION
	"Turn on to enable the possibility to configure VNFs ports (used in the Unify project)"
	OFF
)

IF(ENABLE_UNIFY_PORTS_CONFIGURATION)
	ADD_DEFINITIONS(-DENABLE_UNIFY_PORTS_CONFIGURATION)
ENDIF(ENABLE_UNIFY_PORTS_CONFIGURATION)

OPTION(
	ENABLE_DOUBLE_DECKER_CONNECTION
	"Turn on to connect the orchestrator to the Double Decker network (message bus)"
	OFF
)

IF(ENABLE_DOUBLE_DECKER_CONNECTION)
	ADD_DEFINITIONS(-DENABLE_DOUBLE_DECKER_CONNECTION)
ENDIF(ENABLE_DOUBLE_DECKER_CONNECTION)

OPTION(
	ENABLE_RESOURCE_MANAGER
	"Turn on to activate the 'resource manager')"
	OFF
)

IF(ENABLE_RESOURCE_MANAGER)
	ADD_DEFINITIONS(-DENABLE_RESOURCE_MANAGER)
ENDIF(ENABLE_RESOURCE_MANAGER)


OPTION(
	ENABLE_KVM
	"Turn on to support NFs implemented in KVM-based virtual machines"
	ON
)
IF(ENABLE_KVM)
	ADD_DEFINITIONS(-DENABLE_KVM)
ENDIF(ENABLE_KVM)

OPTION(
	ENABLE-DPDK-CMDLINE-GENERATOR
	"Turn on to compile the code to generate the command line needed to add ivshmem device in the VM"
	OFF
)


OPTION(
		ENABLE_IMOLA_COMPILATION
		"Turn on to support the compilation on imola5"
		OFF
)
IF(ENABLE_IMOLA_COMPILATION)
	ADD_DEFINITIONS(-DENABLE_IMOLA_COMPILATION)
ENDIF(ENABLE_IMOLA_COMPILATION)

#################################################################################################################

OPTION(
	RUN_NFS
	"Turn on the actually run the NFs"
	ON
)
IF(RUN_NFS)
	ADD_DEFINITIONS(-DRUN_NFS)
ENDIF(RUN_NFS)

OPTION(
	ENABLE_DOCKER
	"Turn on the support of NFs implemented in Docker containers"
	OFF
)
IF(ENABLE_DOCKER)
	ADD_DEFINITIONS(-DENABLE_DOCKER)
ENDIF(ENABLE_DOCKER)

OPTION(
	ENABLE_NATIVE
	"Turn on the support of NFs implemented in native form"
	OFF
)
IF(ENABLE_NATIVE)
	ADD_DEFINITIONS(-DENABLE_NATIVE)
ENDIF(ENABLE_NATIVE)


OPTION(ENABLE_DPDK_PROCESSES
	"Turn on the support of NFs implemented in as DPDK processes"
	OFF
)
IF(ENABLE_DPDK_PROCESSES)
	ADD_DEFINITIONS(-DENABLE_DPDK_PROCESSES)
ENDIF(ENABLE_DPDK_PROCESSES)

OPTION(
		ENABLE_NODE_CONFIG
		"Turn on to enable the configuration fo the node"
		OFF
)
IF(ENABLE_NODE_CONFIG)
	ADD_DEFINITIONS(-DENABLE_NODE_CONFIG)
ENDIF(ENABLE_NODE_CONFIG)

OPTION(DEBUG_KVM
	"Turn on the debug of the KVM-libvirt compute plugin"
	OFF
)

IF(DEBUG_KVM)
	ADD_DEFINITIONS(-DDEBUG_KVM)
ENDIF(DEBUG_KVM)

OPTION(
	LOG_1024X768
	"Turn on to print the log for a monitor of 1024 X 768"
	OFF
)
IF(LOG_1024X768)
	ADD_DEFINITIONS(-DLOG_1024X768)
ENDIF(LOG_1024X768)

OPTION(
	DEBUG_OPENFLOW
	"Turn on to enable the debug of Openflow messages"
	OFF
)
IF(DEBUG_OPENFLOW)
	ADD_DEFINITIONS(-DDEBUG_OPENFLOW)
ENDIF(DEBUG_OPENFLOW)


OPTION(
	LOG_ON_FILE
	"Turn on to print log messages on the file 'node-orchestrator.log'. If disabled, stdout is used for the log."
	OFF
)
IF(LOG_ON_FILE)
	ADD_DEFINITIONS(-DLOG_ON_FILE)
ENDIF(LOG_ON_FILE)

OPTION(
	IVSHMEM_RING_ALIAS
	"Use IVSHMEM Ring Aliases to expose localized ring names to VMs (requires DPDK patch)"
	OFF
)
IF(IVSHMEM_RING_ALIAS)
	ADD_DEFINITIONS(-DIVSHMEM_RING_ALIAS)
ENDIF(IVSHMEM_RING_ALIAS)


MARK_AS_ADVANCED(
	DEBUG_KVM DEBUG_OPENFLOW LOG_1024X768 RUN_NFS STARTVNF_SINGLE_THREAD IVSHMEM_RING_ALIAS
)

# relax warning-level (unfortunately):
# some json-spirit header files have unused typedefs
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs")

# Set source files
SET(SOURCES1
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

	node_resource_manager/graph/generic_actions/generic_action.h
	node_resource_manager/graph/generic_actions/generic_action.cc
	node_resource_manager/graph/generic_actions/vlan_action.h
	node_resource_manager/graph/generic_actions/vlan_action.cc
	node_resource_manager/graph/generic_actions/eth_action.h
	node_resource_manager/graph/generic_actions/eth_action.cc
	node_resource_manager/graph/output_actions/output_action.h
	node_resource_manager/graph/output_actions/output_action.cc
	node_resource_manager/graph/output_actions/action_port.h
	node_resource_manager/graph/output_actions/action_port.cc
	node_resource_manager/graph/output_actions/action_network_function.h
	node_resource_manager/graph/output_actions/action_network_function.cc
	node_resource_manager/graph/output_actions/action_endpoint_internal.h
	node_resource_manager/graph/output_actions/action_endpoint_internal.cc
	node_resource_manager/graph/output_actions/action_endpoint_gre.h
	node_resource_manager/graph/output_actions/action_endpoint_gre.cc
	node_resource_manager/graph/output_actions/action_endpoint_hostStack.h
	node_resource_manager/graph/output_actions/action_endpoint_hostStack.cc

	node_resource_manager/graph/graph-parser/graph_parser_utils.h
	node_resource_manager/graph/graph-parser/graph_parser_utils.cc
	node_resource_manager/graph/graph-parser/graph_parser.h
	node_resource_manager/graph/graph-parser/graph_parser.cc
	node_resource_manager/graph/graph-parser/match_parser.h
	node_resource_manager/graph/graph-parser/match_parser.cc
	node_resource_manager/graph/graph-parser/address_validator.h
	node_resource_manager/graph/graph-parser/address_validator.cc

	node_resource_manager/graph/low_level_graph/output_action.h
	node_resource_manager/graph/low_level_graph/output_action.cc
	node_resource_manager/graph/low_level_graph/graph.h
	node_resource_manager/graph/low_level_graph/graph.cc
	node_resource_manager/graph/low_level_graph/low_level_match.h
	node_resource_manager/graph/low_level_graph/low_level_match.cc
	node_resource_manager/graph/low_level_graph/rule.h
	node_resource_manager/graph/low_level_graph/rule.cc

	node_resource_manager/graph/high_level_graph/high_level_action.h
	node_resource_manager/graph/high_level_graph/high_level_action.cc
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
	node_resource_manager/graph/high_level_graph/high_level_graph_endpoint_hostStack.cc
	node_resource_manager/graph/high_level_graph/high_level_graph_endpoint_hostStack.h
	node_resource_manager/graph/high_level_graph/high_level_graph_nodeConfigDefaultGateway.cc
	node_resource_manager/graph/high_level_graph/high_level_graph_nodeConfigDefaultGateway.h
	node_resource_manager/graph/high_level_graph/high_level_graph_vnf.cc
	node_resource_manager/graph/high_level_graph/high_level_graph_vnf.h
	node_resource_manager/graph/high_level_graph/high_level_graph.h
	node_resource_manager/graph/high_level_graph/high_level_graph.cc
	node_resource_manager/graph/high_level_graph/high_level_match.h
	node_resource_manager/graph/high_level_graph/high_level_match.cc
	node_resource_manager/graph/high_level_graph/high_level_element_position.h
	node_resource_manager/graph/high_level_graph/high_level_element_position.cc

	network_controller/switch_manager/switch_manager.h
	network_controller/switch_manager/addNFports_in.h
	network_controller/switch_manager/addNFports_out.h
	network_controller/switch_manager/addVirtualLink_in.h
	network_controller/switch_manager/addVirtualLink_out.h
	network_controller/switch_manager/addEndpointGre_in.h
	network_controller/switch_manager/addEndpointGre_out.h
	network_controller/switch_manager/addEndpointHoststack_in.h
	network_controller/switch_manager/addEndpointHoststack_out.h
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

	compute_controller/template/NFtemplate.h
    compute_controller/template/NFtemplate.cc
	compute_controller/template/port.h
	compute_controller/template/port.cc
	compute_controller/template/template_parser.h
	compute_controller/template/template_parser.cc
	

	node_resource_manager/rest_server/rest_server.h
	node_resource_manager/rest_server/rest_server.cc
	node_resource_manager/rest_server/x-auth-token-header.h
	node_resource_manager/rest_server/x-auth-token-header.cc

	node_resource_manager/database_manager/SQLite/SQLiteManager.h
	node_resource_manager/database_manager/SQLite/SQLiteManager.cc
	node_resource_manager/database_manager/SQLite/SQLiteManager_constants.h

	node_resource_manager/security_manager/security_manager.cc
	node_resource_manager/security_manager/security_manager.h

	node_resource_manager/interface_manager/InterfaceManager.cc
	node_resource_manager/interface_manager/InterfaceManager.h

        node_resource_manager/configuration_agent/ConfigurationAgent.cc
        node_resource_manager/configuration_agent/ConfigurationAgent.h

	utils/logger.h
	utils/logger.c
	utils/constants.h
	utils/sockutils.h
	utils/sockutils.c
	utils/configuration.h
	utils/configuration.cc
)

IF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 0)
    SET(SOURCES2
    ${SOURCES1}
	network_controller/switch_manager/plugins/xdpd/xdpd_manager.h
	network_controller/switch_manager/plugins/xdpd/xdpd_manager.cc
	network_controller/switch_manager/plugins/xdpd/xdpd_constants.h
    )
ENDIF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 0)

IF((VSWITCH_IMPLEMENTATION_INDEX EQUAL 1) OR (VSWITCH_IMPLEMENTATION_INDEX EQUAL 2))
    SET(SOURCES2
    ${SOURCES1}
	network_controller/switch_manager/plugins/ovs-ovsdb/commands.h
	network_controller/switch_manager/plugins/ovs-ovsdb/commands.cc
	network_controller/switch_manager/plugins/ovs-ovsdb/ovsdb_constants.h
	network_controller/switch_manager/plugins/ovs-ovsdb/ovsdb_manager.h
    network_controller/switch_manager/plugins/ovs-ovsdb/ovsdb_manager.cc
    )
ENDIF((VSWITCH_IMPLEMENTATION_INDEX EQUAL 1) OR (VSWITCH_IMPLEMENTATION_INDEX EQUAL 2))

IF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 3)
    SET(SOURCES2
    ${SOURCES1}
	network_controller/switch_manager/plugins/erfs/manager.h
	network_controller/switch_manager/plugins/erfs/manager.cc
	network_controller/switch_manager/plugins/erfs/constants.h
    )
ENDIF(VSWITCH_IMPLEMENTATION_INDEX EQUAL 3)

IF(ENABLE_KVM)
    SET(SOURCES3
    ${SOURCES2}
	compute_controller/plugins/kvm-libvirt/libvirt.cc
	compute_controller/plugins/kvm-libvirt/libvirt.h	
	compute_controller/plugins/kvm-libvirt/ivshmem_cmdline_generator.cc
	compute_controller/plugins/kvm-libvirt/ivshmem_cmdline_generator.h
	)
ELSE()
     SET(SOURCES3
         ${SOURCES2}
    	)
ENDIF(ENABLE_KVM)

IF(ENABLE_DOCKER)
    SET(SOURCES4
    ${SOURCES3}
	compute_controller/plugins/docker/docker.cc
	compute_controller/plugins/docker/docker.h
	compute_controller/plugins/docker/docker_constants.h
	)
ELSE()
     SET(SOURCES4
         ${SOURCES3}
    	)
ENDIF(ENABLE_DOCKER)

IF(ENABLE_DPDK_PROCESSES)
    SET(SOURCES5
    ${SOURCES4}
	compute_controller/plugins/dpdk/dpdk.h
	compute_controller/plugins/dpdk/dpdk.cc
	compute_controller/plugins/dpdk/dpdk_constants.h
	)
ELSE()
     SET(SOURCES5
         ${SOURCES4}
    	)
ENDIF(ENABLE_DPDK_PROCESSES)

IF(ENABLE_NATIVE)
    SET(SOURCES6
    ${SOURCES5}
	compute_controller/plugins/native/native.cc
	compute_controller/plugins/native/native.h
	compute_controller/plugins/native/native_constants.h
	compute_controller/plugins/native/capability.h
	compute_controller/plugins/native/capability.cc
	)
ELSE()
     SET(SOURCES6
         ${SOURCES5}
    	)
ENDIF(ENABLE_NATIVE)

IF(ENABLE_DOUBLE_DECKER_CONNECTION)
	SET(SOURCES7
    ${SOURCES6}
    node_resource_manager/pub_sub/pub_sub_constants.h
    node_resource_manager/pub_sub/pub_sub.h
    node_resource_manager/pub_sub/pub_sub.cc
    )
ELSE()
	SET(SOURCES7
    	${SOURCES6}
    )
ENDIF(ENABLE_DOUBLE_DECKER_CONNECTION)

IF(ENABLE_RESOURCE_MANAGER)
	SET(SOURCES8
    ${SOURCES7}
    node_resource_manager/resource_manager/resource_manager.h
    node_resource_manager/resource_manager/resource_manager.cc
    )
ELSE()
	SET(SOURCES8
		${SOURCES7}
    )
ENDIF(ENABLE_RESOURCE_MANAGER)

INCLUDE_DIRECTORIES (
	${CMAKE_BINARY_DIR}/externals/include/
	/usr/include/boost
	/usr/include/libxml2
)


SET(CMAKE_SKIP_RPATH TRUE)
LINK_DIRECTORIES(
	${CMAKE_BINARY_DIR}/externals/lib/
	)

# Create the executable
ADD_EXECUTABLE(
	node-orchestrator
	${SOURCES8}
)

IF(BUILD_ExternalProjects)
	ADD_DEPENDENCIES(node-orchestrator JSON_Spirit)
	ADD_DEPENDENCIES(node-orchestrator ROFL_Common)
	IF(ENABLE_KVM)
		ADD_DEPENDENCIES(node-orchestrator Libvirt)
	ENDIF(ENABLE_KVM)
ENDIF(BUILD_ExternalProjects)

ADD_DEPENDENCIES(node-orchestrator Inih)

TARGET_LINK_LIBRARIES( node-orchestrator
	pthread
	rofl_common
	json_spirit
	inih
	microhttpd
	boost_system
	xml2
	sqlite3
	crypto
	pistache
	rt
)

IF(ENABLE_DOUBLE_DECKER_CONNECTION)
	TARGET_LINK_LIBRARIES( node-orchestrator
		libdd.so
		libczmq.so
	)
ENDIF(ENABLE_DOUBLE_DECKER_CONNECTION)

#Links specific for KVM
IF(ENABLE_KVM)
	TARGET_LINK_LIBRARIES( node-orchestrator
		libvirt.so
	)
ENDIF(ENABLE_KVM)

###############################################################################

#Add configuration rules to compile command line generator

IF(ENABLE-DPDK-CMDLINE-GENERATOR)

	IF(NOT DEFINED ENV{RTE_SDK})
		message(FATAL_ERROR "Please define RTE_SDK environment variable (it must point to the DPDK folder)")
	ENDIF(NOT DEFINED ENV{RTE_SDK})
	
	IF(NOT DEFINED ENV{RTE_TARGET})
		message(FATAL_ERROR "Please define RTE_TARGET environment variable as follows: 'export RTE_TARGET=x86_64-ivshmem-linuxapp-gcc'")
	ENDIF(NOT DEFINED ENV{RTE_TARGET})

	ADD_DEPENDENCIES(node-orchestrator cmd_line_generator)

	IF (IVSHMEM_RING_ALIAS)
		SET(CMDLINEGEN_MAKE_PREARGS EXTRA_CFLAGS="-DIVSHMEM_RING_ALIAS")
	ENDIF(IVSHMEM_RING_ALIAS)
	  
	ADD_CUSTOM_TARGET(
		cmd_line_generator
		COMMAND ${CMDLINEGEN_MAKE_PREARGS} make
		WORKING_DIRECTORY compute_controller/plugins/kvm-libvirt/cmdline_generator/
		)

ENDIF(ENABLE-DPDK-CMDLINE-GENERATOR)

###############################################################################

#Check unsupported configurations

IF(ENABLE_RESOURCE_MANAGER AND (NOT ENABLE_DOUBLE_DECKER_CONNECTION))
	#The resource manager requires the DD connection
	message(FATAL_ERROR "Resource manager requires to enable the double decker connection")
ENDIF(ENABLE_RESOURCE_MANAGER AND (NOT ENABLE_DOUBLE_DECKER_CONNECTION))

IF(ENABLE_DPDK_PROCESSES AND ( ${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 2) )
	#This configuration (ovsdb + dpdk processes) is not supported
	message(FATAL_ERROR "OvS not supported with Dpdk processes")
ENDIF(ENABLE_DPDK_PROCESSES AND ( ${VSWITCH_IMPLEMENTATION_INDEX} EQUAL 2) )

IF( (NOT ENABLE_DOCKER) AND (NOT ENABLE_NATIVE) AND (NOT ENABLE_DPDK_PROCESSES) AND (NOT ENABLE_KVM) )
	#No execution environment is selected
	message(FATAL_ERROR "At least one execution environmnet must be enabled")
ENDIF( (NOT ENABLE_DOCKER) AND (NOT ENABLE_NATIVE) AND (NOT ENABLE_DPDK_PROCESSES) AND (NOT ENABLE_KVM) )

###############################################################################

# Build external projects

include (ExternalProject)
IF(BUILD_ExternalProjects)

	# rofl-common: extract, build, install (locally in build-dir):
	ExternalProject_Add(
		ROFL_Common
		URL ${CMAKE_SOURCE_DIR}/contrib/rofl-common.zip
		URL_MD5 37b524426d1c7ee9c63e01c8f467d4e2
		BUILD_IN_SOURCE 1
		CONFIGURE_COMMAND ./autogen.sh && ./configure --prefix=${CMAKE_BINARY_DIR}/externals
	)

	IF(ENABLE_KVM)
		# libvirt - specific revision
		ExternalProject_Add( Libvirt

			## this refers to the specific revision from README_COMPILE.md
			# GIT_REPOSITORY git://libvirt.org/libvirt.git
			# GIT_TAG f57842ecfda1ece8c59718e62464e17f75a27062

			# try a regular release instead:
			URL http://libvirt.org/sources/libvirt-1.2.17.tar.gz
			URL_MD5 a1f49050223be3cbd7678c32b1ee2756

			BUILD_IN_SOURCE 1
			CONFIGURE_COMMAND ./autogen.sh --prefix=${CMAKE_BINARY_DIR}/externals
			UPDATE_COMMAND ""
			ALWAYS 0
		)
	ENDIF(ENABLE_KVM)

ENDIF(BUILD_ExternalProjects)

