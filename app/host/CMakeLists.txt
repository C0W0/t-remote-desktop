add_executable(host)

target_sources(host
        PRIVATE
            main.cpp
)

#target_link_libraries(host PRIVATE
#    network
#)

if(WIN32)
    target_link_libraries(host PRIVATE ws2_32)
endif()