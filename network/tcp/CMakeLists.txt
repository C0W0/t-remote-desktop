target_sources(network
    PRIVATE
        src/Server.cpp
        src/Socket.cpp
)

if (WIN32)
    target_sources(network
        PRIVATE
            src/windows/ConnectionSocketWinImpl.cpp
            src/windows/ListeningSocketWinImpl.cpp
    )
    target_link_libraries(network PRIVATE ws2_32)
else ()
    target_sources(network
            PRIVATE
            src/posix/ConnectionSocketPosixImpl.cpp
            src/posix/ListeningSocketPosixImpl.cpp
    )
endif ()

target_include_directories(network
    PUBLIC
        include
)