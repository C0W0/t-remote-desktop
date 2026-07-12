add_executable(client)

target_sources(client
    PRIVATE
        main.cpp
)

#target_link_libraries(client PRIVATE
#    network
#)


if (WIN32)
    target_link_libraries(client PRIVATE ws2_32)
endif ()