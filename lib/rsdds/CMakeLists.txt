# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8.0)  # source_group(TREE)
project( rsdds )

add_library( ${PROJECT_NAME} STATIC "" )
target_link_libraries( ${PROJECT_NAME} PUBLIC rscore realdds )
set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER Library )

target_include_directories( ${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Headers -----------------------------------------------------------------------------------
#
set( RSDDS_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/rsdds" )
file(GLOB_RECURSE RSDDS_HEADER_FILES
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    "${RSDDS_INCLUDE_DIR}/*.h"
)
target_sources( ${PROJECT_NAME} PRIVATE ${RSDDS_HEADER_FILES} )
source_group(
    TREE ${RSDDS_INCLUDE_DIR}
    PREFIX "Header Files"
    FILES ${RSDDS_HEADER_FILES} )


# Sources -----------------------------------------------------------------------------------
#
set( RSDDS_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src" )
file(GLOB_RECURSE RSDDS_SOURCE_FILES
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    "${RSDDS_SRC_DIR}/*"
)
target_sources( ${PROJECT_NAME} PRIVATE ${RSDDS_SOURCE_FILES} )
source_group(
    TREE ${RSDDS_SRC_DIR}
    PREFIX "Source Files"
    FILES ${RSDDS_SOURCE_FILES} )


# Share -------------------------------------------------------------------------------------
#
register_rscore_module( ${PROJECT_NAME} )


# Install -----------------------------------------------------------------------------------
#
install( TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )


# Examples -----------------------------------------------------------------------------------
#
if( BUILD_EXAMPLES )
    add_subdirectory( examples )
endif()
