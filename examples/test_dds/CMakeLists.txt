
cmake_minimum_required(VERSION 3.16.3)

project("test_dds")


include( ${CMAKE_SOURCE_DIR}/CMake/opengl_config.cmake )


add_executable( ${PROJECT_NAME}
    Device.cxx
    Host.cxx
    Udds.cxx
    UddsPubSubTypes.cxx
    HostEvent.cpp
    test_dds.cpp
    ../../third-party/imgui/imgui.cpp
    ../../third-party/imgui/imgui_draw.cpp
    ../../third-party/imgui/imgui_impl_glfw.cpp
    )

target_link_libraries( ${PROJECT_NAME} PRIVATE realdds glfw ${OPENGL_LIBRARIES} )

include_directories(
    ../../common
    ../../third-party/imgui
    ../../examples
    )

set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER Examples )

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

using_easyloggingpp( ${PROJECT_NAME} )

