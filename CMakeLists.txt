cmake_minimum_required (VERSION 2.6)
project ( CCollections C)
add_definitions ( "-D_CCollections" )
set ( _CCollections 1 )

if(CMAKE_SIZEOF_VOID_P EQUAL 8) 	# 64 bits
    set ( ARCH_NAME "x64" )
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
    set ( ARCH_NAME "x86" )
endif()

#set( LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/${ARCH_NAME}/${CMAKE_BUILD_TYPE}" )

#include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
#list(APPEND sourceFiles
#    "include/CCollections/CList.h"
#)
#source_group ( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sourceFiles} )
#target_sources( CCollections STATIC ${sourceFiles} )

#add_library ( CCollections INTERFACE )
#target_include_directories(CCollections INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include" )


#target_include_directories ( CCollections PUBLIC "include" )
add_library( CCollections INTERFACE )
target_include_directories( CCollections INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include" )
