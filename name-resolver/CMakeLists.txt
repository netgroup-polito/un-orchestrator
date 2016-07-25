CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(name-resolver)

# From  http://gcc.gnu.org/onlinedocs/gcc/i386-and-x86_002d64-Options.html
#
#-mvect8-ret-in-mem
#    Return 8-byte vectors in memory instead of MMX registers. This is the 
#    default on Solaris 8 and 9 and VxWorks to match the ABI of the Sun Studio
#    compilers until version 12. Later compiler versions (starting with Studio 
#    12 Update 1) follow the ABI used by other x86 targets, which is the default 
#    on Solaris 10 and later. Only use this option if you need to remain compatible
#    with existing code produced by those previous compiler versions or older 
#    versions of GCC. 
#
#
#-march=corei7-avx
#
#    -march=cpu-type
#       Generate instructions for the machine type cpu-type. In contrast to -mtune=cpu-type, 
#       which merely tunes the generated code for the specified cpu-type, -march=cpu-type 
#       allows GCC to generate code that may not run at all on processors other than the 
#       one indicated. Specifying -march=cpu-type implies -mtune=cpu-type. 
#
#    corei7-avx
#      Intel Core i7 CPU with 64-bit extensions, MMX, SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, 
#      AVX, AES and PCLMUL instruction set support. 
#
# WARNING: if you are not using an Intel Core i7 CPU, please do not use this flag, otherwise
#  the code cannot be compiled.
#
# Options that control optimizations in gcc are described here:
#     http://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
#

IF(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUCC)
	ADD_DEFINITIONS(-Wall -Werror -Wno-write-strings -O3)
	IF(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
		ADD_DEFINITIONS(-mvect8-ret-in-mem -march=corei7-avx)# -fbranch-probabilities)
	ENDIF()
ENDIF()

# Rather complicated CMake code for setting the logging level
SET(LOGGING_LEVEL "ORCH_WARNING" CACHE STRING "Orchestrator logging levels: ORCH_DEBUG, ORCH_DEBUG_INFO, ORCH_WARNING, ORCH_ERROR, ORCH_INFO")
SET(LOGGING_LEVEL_VALUES "ORCH_DEBUG" "ORCH_DEBUG_INFO" "ORCH_WARNING" "ORCH_ERROR" "ORCH_INFO")
SET_PROPERTY(CACHE LOGGING_LEVEL PROPERTY STRINGS ${LOGGING_LEVEL_VALUES})
LIST(FIND LOGGING_LEVEL_VALUES ${LOGGING_LEVEL} LOGGING_LEVEL_INDEX)

IF(${LOGGING_LEVEL_INDEX} EQUAL -1)
	message(FATAL_ERROR "Option ${LOGGING_LEVEL} not supported, valid entries are ${LOGGING_LEVEL_VALUES}")
ENDIF()
IF(${LOGGING_LEVEL_INDEX} EQUAL 0)
	ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_DEBUG)
ENDIF()
IF(${LOGGING_LEVEL_INDEX} EQUAL 1)
	ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_DEBUG_INFO)
ENDIF()
IF(${LOGGING_LEVEL_INDEX} EQUAL 2)
	ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_WARNING)
ENDIF()
IF(${LOGGING_LEVEL_INDEX} EQUAL 3)
	ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_ERROR)
ENDIF()
IF(${LOGGING_LEVEL_INDEX} EQUAL 4)
	ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_INFO)
ENDIF()
# End of the rather complicated CMake code for setting the logging level

OPTION(
	LOG_ON_FILE
	"Turn on to print log messages on the file 'name-resolver.log'. If disabled, stdout is used for the log."
	OFF
)
IF(LOG_ON_FILE)
	ADD_DEFINITIONS(-DLOG_ON_FILE)
ENDIF(LOG_ON_FILE)

# Set source files
SET(SOURCES
	name_resolver.cc	
	
	rest_server.h
	rest_server.cc
	
	nf.h
	nf.cc
	implementation.h
	implementation.cc
	
	logger.h
	logger.c
	
	constants.h
)

INCLUDE_DIRECTORIES (
	${CMAKE_BINARY_DIR}/externals/include/
	/usr/include/libxml2
)

SET(CMAKE_SKIP_RPATH TRUE)
LINK_DIRECTORIES(
	${CMAKE_BINARY_DIR}/externals/lib/
)

# Create the executable
ADD_EXECUTABLE(
	name-resolver
	${SOURCES}
)

ADD_DEPENDENCIES(name-resolver JSON_Spirit)


TARGET_LINK_LIBRARIES( name-resolver
	pthread
	xml2
	json_spirit
	microhttpd
	rt
)

