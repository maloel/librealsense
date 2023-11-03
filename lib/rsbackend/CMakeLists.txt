# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8.0)  # source_group(TREE)
project( rsbackend )

add_library( ${PROJECT_NAME} STATIC "" )
target_link_libraries( ${PROJECT_NAME} PUBLIC rscore )
set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER Library )

target_include_directories( ${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Headers -----------------------------------------------------------------------------------
#
set( RSBACKEND_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/rsbackend" )
file(GLOB_RECURSE RSBACKEND_HEADER_FILES
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    "${RSBACKEND_INCLUDE_DIR}/*.h"
)
target_sources( ${PROJECT_NAME} PRIVATE ${RSBACKEND_HEADER_FILES} )
source_group(
    TREE ${RSBACKEND_INCLUDE_DIR}
    PREFIX "Header Files"
    FILES ${RSBACKEND_HEADER_FILES} )


# Sources -----------------------------------------------------------------------------------
#
set( RSBACKEND_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src" )
file(GLOB_RECURSE RSBACKEND_SOURCE_FILES
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    "${RSBACKEND_SRC_DIR}/*"
)
target_sources( ${PROJECT_NAME} PRIVATE ${RSBACKEND_SOURCE_FILES} )
source_group(
    TREE ${RSBACKEND_SRC_DIR}
    PREFIX "Source Files"
    FILES ${RSBACKEND_SOURCE_FILES} )


# Share -------------------------------------------------------------------------------------
#
register_rscore_module( ${PROJECT_NAME} )


# Install -----------------------------------------------------------------------------------
#
install( TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
