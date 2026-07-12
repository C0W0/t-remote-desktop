target_sources(network
    PRIVATE
        src/Server.cpp
        src/ServerSocket.cpp
)

if (WIN32)
    target_sources(network
        PRIVATE
            src/windows/WindowServerSocket.cpp
    )
    target_link_libraries(network PRIVATE ws2_32)
endif ()

target_include_directories(network
    PUBLIC
        include
)