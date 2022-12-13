# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

project(poc-h)

set( SRC
    poc-h.cpp

    ../payload/op-payload.h
    ../payload/op-payload.cpp
    ../payload/op-reader.h
    ../payload/op-reader.cpp
    ../payload/op-writer.h
    ../payload/op-writer.cpp

    ../payload/stream-payload.h
    ../payload/stream-payload.cpp
    ../payload/stream-reader.h
    ../payload/stream-reader.cpp

    ../payload/realsense_types.h
    ../payload/realsense_types.cxx
    ../payload/realsense_typesPubSubTypes.h
    ../payload/realsense_typesPubSubTypes.cxx
    )

add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE realdds )
target_include_directories( ${PROJECT_NAME} PRIVATE
    ${REPO_ROOT}/include
    ${REPO_ROOT}/third-party/tclap/include
    ..
)
set_target_properties (${PROJECT_NAME} PROPERTIES
                       FOLDER Tools/dds
                       CXX_STANDARD 11)

include("${REPO_ROOT}/include/librealsense2/utilities/string/CMakeLists.txt")
include("${REPO_ROOT}/include/librealsense2/utilities/easylogging/easyloggingpp.cmake")
include("${REPO_ROOT}/include/librealsense2/utilities/concurrency/CMakeLists.txt")
target_sources( ${PROJECT_NAME} PRIVATE ${ELPP_SOURCE_FILES} ${CONCURRENCY_SOURCE_FILES} )
source_group( "EasyLogging++" FILES ${ELPP_SOURCE_FILES}  )

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

