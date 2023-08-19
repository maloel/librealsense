# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8.0)  # source_group(TREE)
project( rscore )

add_library( ${PROJECT_NAME} STATIC "" )
target_link_libraries( ${PROJECT_NAME} PUBLIC rsutils )
set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER Library )

target_include_directories( ${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Headers -----------------------------------------------------------------------------------
#
set( RSCORE_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/rscore" )
file(GLOB_RECURSE RSCORE_HEADER_FILES
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    "${RSCORE_INCLUDE_DIR}/*.h"
)
target_sources( ${PROJECT_NAME} PRIVATE ${RSCORE_HEADER_FILES} )
source_group(
    TREE ${RSCORE_INCLUDE_DIR}
    PREFIX "Header Files"
    FILES ${RSCORE_HEADER_FILES} )


# Sources -----------------------------------------------------------------------------------
#
set( RSCORE_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src" )
file(GLOB_RECURSE RSCORE_SOURCE_FILES
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    "${RSCORE_SRC_DIR}/*"
)
target_sources( ${PROJECT_NAME} PRIVATE ${RSCORE_SOURCE_FILES} )
source_group(
    TREE ${RSCORE_SRC_DIR}
    PREFIX "Source Files"
    FILES ${RSCORE_SOURCE_FILES} )



# Install -----------------------------------------------------------------------------------
#
install( TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )


# Python ------------------------------------------------------------------------------------
#
#if( BUILD_PYTHON_BINDINGS )
#    add_subdirectory( py )
#endif()

