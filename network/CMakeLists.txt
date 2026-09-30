add_library(network)

add_subdirectory(tcp)
add_subdirectory(protocol)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()
