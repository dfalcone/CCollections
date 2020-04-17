cmake_minimum_required ( VERSION 2.6 )
project ( CExamples C)

#if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
#    set(CMAKE_SYSTEM_VERSION "10.0.17763.0")
#endif()


set ( OBJ_DIR "obj" )
if (CMAKE_VS_PLATFORM_NAME)
    set ( OBJ_DIR ${CMAKE_VS_PLATFORM_NAME} )
endif()

#add_subdirectory ( "../../../CCollections"       "${CMAKE_BINARY_DIR}/${OBJ_DIR}/CCollections"      )

set( CCOLLECTIONS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../include")
include_directories( ${CCOLLECTIONS_INCLUDE_DIR} )

list(APPEND ccollectionsFiles
    "${CCOLLECTIONS_INCLUDE_DIR}/CCollections/CTypes.h"
    "${CCOLLECTIONS_INCLUDE_DIR}/CCollections/CList.h"
)
source_group ( TREE "${CCOLLECTIONS_INCLUDE_DIR}" FILES ${ccollectionsFiles} )

list(APPEND sourceFiles
    CExamples.c
)

add_executable ( CExamples ${sourceFiles} ${ccollectionsFiles} )
set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT CExamples )

#target_link_libraries ( CExamples INTERFACE CCollections )
#target_include_directories(  CExamples INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/CCollections )
#target_include_directories( CExamples INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/CCollections)

#target_link_libraries ( CExamples    LINK_PUBLIC     CCollections       )

set_target_properties( CExamples PROPERTIES LINKER_LANGUAGE C )
set_target_properties( CExamples PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" )
set_target_properties( CExamples PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" )

mark_as_advanced( FORCE CMAKE_INSTALL_PREFIX ) # not supporting cmake install

if(MSVC)
  target_compile_options(CExamples PRIVATE /Wall /WX) #w4
else()
  target_compile_options(CExamples PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

message ( STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message ( STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
message ( STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message ( STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

# install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data" DESTINATION "${CMAKE_BINARY_DIR}/data" COMPONENT runtime)
