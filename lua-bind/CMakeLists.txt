MESSAGE("Building lhtimer wrapper")

FILE(GLOB_RECURSE LTIMER_C_FILES ${CMAKE_SOURCE_DIR}/htimer*.c ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/)

SET(CMAKE_SHARED_LINKER_FLAGS "-dynamiclib -Wl,-undefined,dynamic_lookup")
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

LIST(REMOVE_ITEM LTIMER_C_FILES ${CMAKE_SOURCE_DIR}/test/test.c)


ADD_LIBRARY(lhtimer SHARED ${LTIMER_C_FILES})
set_target_properties(lhtimer PROPERTIES OUTPUT_NAME "lhtimer")
set_target_properties(lhtimer PROPERTIES SUFFIX ".so")
set_target_properties(lhtimer PROPERTIES PREFIX "")

configure_file(${CMAKE_SOURCE_DIR}/lua-bind/test.lua test.lua)

