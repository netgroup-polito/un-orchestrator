CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(db_initializer)

IF(CMAKE_COMPILER_IS_GNUCXX)
        ADD_DEFINITIONS(-Wall -Werror -Wno-write-strings)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)
IF(CMAKE_COMPILER_IS_GNUCC)
        ADD_DEFINITIONS(-Wall -Werror -Wno-write-strings)
ENDIF(CMAKE_COMPILER_IS_GNUCC)

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
	${CMAKE_BINARY_DIR}/externals/include/
	/usr/include/boost
)

SET(CMAKE_SKIP_RPATH TRUE)
LINK_DIRECTORIES(
	${CMAKE_BINARY_DIR}/externals/lib/
	)

# Create the executable
ADD_EXECUTABLE(
	db_initializer
	${SOURCES}
)

ADD_DEPENDENCIES(db_initializer Inih)


TARGET_LINK_LIBRARIES( db_initializer
	inih
	rt
	sqlite3
	ssl
	crypto
	boost_system
)
