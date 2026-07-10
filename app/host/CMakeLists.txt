add_executable(host)

target_sources(host
        PRIVATE
            main.cpp
)

target_link_libraries(host PRIVATE
    network
)