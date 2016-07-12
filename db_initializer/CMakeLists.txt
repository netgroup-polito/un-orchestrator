CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(db_initializer)

IF(CMAKE_COMPILER_IS_GNUCXX)
        ADD_DEFINITIONS(-Wall -Werror -Wno-write-strings -mvect8-ret-in-mem -march=corei7-avx -O3)# -fbranch-probabilities)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)
IF(CMAKE_COMPILER_IS_GNUCC)
        ADD_DEFINITIONS(-Wall -Werror -Wno-write-strings -mvect8-ret-in-mem -march=corei7-avx -O3)# -fbranch-probabilities)
ENDIF(CMAKE_COMPILER_IS_GNUCC)


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
	db_initializer.cc	
	
	../orchestrator/node_resource_manager/database_manager/SQLite/SQLiteManager.cc
	../orchestrator/node_resource_manager/database_manager/SQLite/SQLiteManager.h
	
	../orchestrator/utils/logger.c
	../orchestrator/utils/logger.h
	../orchestrator/utils/constants.h
)

INCLUDE_DIRECTORIES (
	/usr/include/libxml2
)

# Create the executable
ADD_EXECUTABLE(
	db_initializer
	${SOURCES}
)



LINK_DIRECTORIES(
	/usr/lib/
	/usr/local/lib/
	/usr/x86_64-linux-gnu/
	/usr/lib/x86_64-linux-gnu/
)


TARGET_LINK_LIBRARIES( db_initializer
	libpthread.so
	libxml2.so
	libjson_spirit.so
	libmicrohttpd.so
	-lrt
	-lsqlite3
	-lssl
	-lcrypto
	-lboost_system
)