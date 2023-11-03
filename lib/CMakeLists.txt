
add_subdirectory( "${CMAKE_CURRENT_LIST_DIR}/rscore" )
add_subdirectory( "${CMAKE_CURRENT_LIST_DIR}/rsbackend" )
if( BUILD_WITH_DDS )
    add_subdirectory( "${CMAKE_CURRENT_LIST_DIR}/rsdds" )
endif()
