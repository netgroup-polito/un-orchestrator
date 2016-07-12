set( CMAKE_SYSTEM_NAME Linux )
set( CMAKE_SYSTEM_PROCESSOR mips )
set( CMAKE_C_COMPILER mips-openwrt-linux-gcc )
set( CMAKE_CXX_COMPILER mips-openwrt-linux-g++ )
set( CMAKE_FIND_ROOT_PATH [KERNEL] )

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
