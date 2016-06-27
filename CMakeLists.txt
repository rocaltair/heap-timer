#INCLUDE(/path/to/another/CMakeLists.txt)
cmake_minimum_required(VERSION 2.5)
set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall")

project(proj_htimer)
set(LIBNAME "htimer")

include_directories(
	.
)

#option(ENABLE_TCMALLOC "enable tcmalloc of google" ON)

option(ENABLE_CTEST "C_TEST" ON)
option(ENABLE_LUA "LUA_BIND" ON)

IF (ENABLE_LUA)
	ADD_SUBDIRECTORY(lua-bind)
ENDIF (ENABLE_LUA)


IF (ENABLE_CTEST)
	ADD_SUBDIRECTORY(test)
ENDIF (ENABLE_CTEST)

#add_subdirectory(SRC_DIR
#	subdir
#)

SET(LIB_LIST
	htimer.c
)

ADD_LIBRARY(${LIBNAME} STATIC
	${LIB_LIST}
)

MESSAGE("== <CMAKE_SYSTEM_NAME> " ${CMAKE_SYSTEM_NAME})

FOREACH(item ${LIB_LIST})
	MESSAGE("--<LIBRARY> " ${item})
ENDFOREACH(item)
