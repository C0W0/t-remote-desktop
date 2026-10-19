target_sources(network
    PRIVATE
        src/Transport.cpp
        src/client/Auth.cpp
        src/client/Control.cpp
        src/server/Image.cpp
        src/server/Response.cpp
        src/shared/Disconnect.cpp
        src/shared/Message.cpp
)

target_link_libraries(network
    PUBLIC
        magic_enum::magic_enum
)

target_include_directories(network
    PUBLIC
        include
)