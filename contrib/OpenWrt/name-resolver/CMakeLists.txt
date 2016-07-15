CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(name-resolver)

ADD_DEFINITIONS(-Wall -Werror -Wno-write-strings -O3)# -fbranch-probabilities)

ADD_DEFINITIONS(-DLOGGING_LEVEL=ORCH_DEBUG_INFO)
# End of the rather complicated CMake code for setting the logging level

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
	/usr/local/include
	/usr/local/lib
	/usr/include/boost
	/usr/include/libxml2
)

# Create the executable
ADD_EXECUTABLE(
	name-resolver
	${SOURCES}
)



LINK_DIRECTORIES(
	/usr/lib/
	/usr/local/lib/
	/usr/x86_64-linux-gnu/
	/usr/lib/x86_64-linux-gnu/
)


TARGET_LINK_LIBRARIES( name-resolver
	libpthread.so
	libxml2.so
	libjson_spirit.so
	libmicrohttpd.so
	-lrt
)

