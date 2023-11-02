# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.
cmake_minimum_required( VERSION 3.10 )

project( rsdds-devices )

add_executable( ${PROJECT_NAME} ${PROJECT_NAME}.cpp )
set_target_properties( ${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 14
    FOLDER "Examples/DDS"
    )
target_link_libraries( ${PROJECT_NAME} PRIVATE rsdds tclap )

install( TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
