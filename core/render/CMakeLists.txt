add_library(render)

set(SOURCE_LIST
    src/Frame.cpp
)

if (APPLE)
    list(APPEND SOURCE_LIST src/metal/MetalFrame.cpp src/metal/MetalRenderer.mm)
    target_link_libraries(render
        PRIVATE
            "-framework Foundation"
            "-framework Metal"
            "-framework QuartzCore"
    )
endif ()

target_sources(render
    PRIVATE
        ${SOURCE_LIST}
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