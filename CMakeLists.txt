CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

OPTION(BUILD_ExternalProjects "Build external projects (json-spirit, rofl, ...)" ON)
OPTION(BUILD_Orchestrator "Build orchestrator" ON)
OPTION(BUILD_NameResolver "Build name-resolver" ON)

include (ExternalProject)
IF(BUILD_ExternalProjects)
# json-spirit: extract, build, install (locally in build-dir):
ExternalProject_Add(
	JSON_Spirit
	URL ${CMAKE_SOURCE_DIR}/contrib/json-spirit.zip
	URL_MD5 728c247d012bb999dca38629bb4649aa
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/externals
	# unfortunately json-spirit lacks a top-level cmakelists.txt:
	PATCH_COMMAND printf "CMAKE_MINIMUM_REQUIRED(VERSION 2.8)\\nADD_SUBDIRECTORY(build)\\n" > CMakeLists.txt
)
ENDIF(BUILD_ExternalProjects)

IF(BUILD_Orchestrator)
	ADD_SUBDIRECTORY(orchestrator)
ENDIF(BUILD_Orchestrator)

IF(BUILD_NameResolver)
	ADD_SUBDIRECTORY(name-resolver)
ENDIF(BUILD_NameResolver)
