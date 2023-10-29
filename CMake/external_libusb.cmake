# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

include(ExternalProject)

message( STATUS "Fetching libusb..." )

function( get_libusb )

    configure_file( ${CMAKE_SOURCE_DIR}/CMake/libusb-download.cmake.in
                    ${CMAKE_BINARY_DIR}/external-projects/libusb-download/CMakeLists.txt )
    execute_process( COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
                     -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                     -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                     -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/external-projects/libusb-download"
                     #OUTPUT_QUIET
                     RESULT_VARIABLE configure_ret )
    execute_process( COMMAND "${CMAKE_COMMAND}" --build .
                     WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/external-projects/libusb-download"
                     #OUTPUT_QUIET
                     RESULT_VARIABLE build_ret )

    if( configure_ret OR build_ret )
        message( FATAL_ERROR "Failed to download libusb" )
    endif()

    add_subdirectory( "${CMAKE_BINARY_DIR}/third-party/libusb/" "${CMAKE_BINARY_DIR}/third-party/libusb/build" )

    add_library(usb INTERFACE)
    target_include_directories( usb INTERFACE
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/third-party/libusb/libusb>
        )
    target_link_libraries( usb INTERFACE
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/third-party/libusb/build/$<CONFIG>/${CMAKE_STATIC_LIBRARY_PREFIX}usb${CMAKE_STATIC_LIBRARY_SUFFIX}>
        )
    set(USE_EXTERNAL_USB ON) # INTERFACE libraries can't have real deps, so targets that link with usb need to also depend on libusb

    if (APPLE)
      find_library(corefoundation_lib CoreFoundation)
      find_library(iokit_lib IOKit)
      target_link_libraries(usb INTERFACE objc ${corefoundation_lib} ${iokit_lib})
    endif()

endfunction()
get_libusb()

message( STATUS "Fetching libusb - Done" )

