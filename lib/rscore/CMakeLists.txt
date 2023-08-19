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


# Macros ------------------------------------------------------------------------------------
#
macro( _register_rscore_module )
    #
    # register_rscore_module <target>
    #
    # Projects that need to register their own modules for use by rscore's context should
    # call this function. The C++ code should likewise use the REGISTER_RSCORE_MODULE macro to
    # properly register the module in any .cpp file.
    #
    function( register_rscore_module target )
        # fARGN holds the list of arguments past the last expected argument
        set( fARGN ARGN )  # otherwise we get the outside macro's ARGN
        set( fARGC ARGC )

        if( ${${fARGC}} GREATER 1 )
            message( FATAL_ERROR "too many (${${fARGC}}) arguments; usage: register_rscore_module <target>" )
        endif()

        # The module name is made available to C++ through a definition _RSCORE_MODULE_NAME:
        #
        target_compile_definitions( ${target} PRIVATE _RSCORE_MODULE_NAME=${target} )
        #
        # The registry entry is statically initialized but not usually referenced anywhere, which
        # means the linker will try to remove it! If it does it will not be included and no module
        # will be initialized... so we tell any dependents to not remove the module entry:
        #
        # In Windows (Visual Studio):
        #       /INCLUDE (Force Symbol References)
        #       "tells the linker to add a specified symbol to the symbol table ... overrides the
        #       removal of that symbol by /OPT:REF"
        #       (NOTE: target_link_libraries requires a '-' and, luckily, the linker accepts it)
        # Linux:
        #       "Force symbol to be entered in the output file as an undefined symbol"
        #       This will cause a particular symbol to be pulled into the link.
        #
        if( WIN32 )
            target_link_libraries( ${target} INTERFACE -INCLUDE:${target}_registry_entry )
        else()
            target_link_libraries( ${target} INTERFACE --undefined=${target}_registry_entry )
        endif()
    endfunction()
endmacro()
_register_rscore_module()
