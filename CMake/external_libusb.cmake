# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

include(ExternalProject)

message( STATUS "Fetching libusb..." )


ExternalProject_Add(
    libusb

    # Work-around for libusb master broken on Nov 26' 2020 with introduction of v1.0.24
    # the issue has been reported in https://github.com/libusb/libusb/issues/812
    GIT_REPOSITORY "https://github.com/ev-mp/libusb.git"
    GIT_TAG "2a7372db54094a406a755f0b8548b614ba8c78ec" # "v1.0.22" + Mac get_device_list hang fix

    SOURCE_DIR "${CMAKE_BINARY_DIR}/third-party/libusb/"
    BINARY_DIR "${CMAKE_BINARY_DIR}/third-party/libusb/build"

    UPDATE_COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/third-party/libusb/CMakeLists.txt
            ${CMAKE_BINARY_DIR}/third-party/libusb/CMakeLists.txt
    PATCH_COMMAND ""
    TEST_COMMAND ""

    CMAKE_ARGS -DCMAKE_CXX_STANDARD_LIBRARIES=${CMAKE_CXX_STANDARD_LIBRARIES}
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DANDROID_ABI=${ANDROID_ABI}
            -DANDROID_STL=${ANDROID_STL}
)

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

message( STATUS "Fetching libusb - Done" )

