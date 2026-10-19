add_executable(host)

target_sources(host
    PRIVATE
        src/main.cpp
        src/ServerApp.cpp
)

target_include_directories(host
    PRIVATE
        include
)

target_link_libraries(host PRIVATE
    network
)