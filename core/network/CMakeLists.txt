add_library(network)

add_subdirectory(src)

target_include_directories(network
    PUBLIC
        include
)