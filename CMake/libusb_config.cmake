# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.

if (NOT TARGET usb)
    find_library(LIBUSB_LIB usb-1.0)
    find_path(LIBUSB_INC libusb.h HINTS PATH_SUFFIXES libusb-1.0)
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(usb "libusb not found; using internal version" LIBUSB_LIB LIBUSB_INC)
    if (USB_FOUND AND NOT USE_EXTERNAL_USB)
        add_library(usb INTERFACE)
        target_include_directories(usb INTERFACE ${LIBUSB_INC})
        target_link_libraries(usb INTERFACE ${LIBUSB_LIB})
    else()
        include( ${CMAKE_SOURCE_DIR}/CMake/external_libusb.cmake )
    endif()
    install(TARGETS usb EXPORT realsense2Targets)
endif()
