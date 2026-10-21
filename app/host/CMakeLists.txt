add_executable(host)

target_sources(host
    PRIVATE
        src/main.cpp
        src/server_app/ServerApp.cpp
)

target_link_libraries(host
    PRIVATE
        network
        capture
)