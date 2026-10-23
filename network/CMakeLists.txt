add_library(network)
target_link_libraries(network PUBLIC common)

add_subdirectory(tcp)
add_subdirectory(protocol)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()
