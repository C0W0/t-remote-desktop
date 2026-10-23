add_library(render)

target_sources(render
    PRIVATE
        src/Frame.cpp
)

target_include_directories(render
    PUBLIC
        include
)

target_link_libraries(render
    PUBLIC
        SDL3::SDL3
)

add_executable(render_demo)

target_sources(render_demo
    PRIVATE
        src/main.cpp
)

target_link_libraries(render_demo
    PRIVATE
        render
)