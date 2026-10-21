add_library(capture)

target_sources(capture
    PRIVATE
        src/Capturer.cpp
)

target_include_directories(capture
    PUBLIC
        include
)

target_link_libraries(capture
    PUBLIC
        common
)