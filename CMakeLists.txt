cmake_minimum_required(VERSION 4.1)
project(trdp)

set(CMAKE_CXX_STANDARD 23)

include(CTest) # defines BUILD_TESTING (ON by default) and calls enable_testing()
if (BUILD_TESTING)
    # GoogleTest is fetched once here; each module declares its own <module>_tests in <module>/tests.
    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    # Windows: use the shared CRT, matching the parent project
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)
endif ()

add_subdirectory(network)
add_subdirectory(app)

# Enforces the include-ordering convention: system headers above project headers
# in .h files, below them in .cpp files. This cannot live in .clang-format --
# IncludeCategories priorities are global to a format run and cannot vary by file
# extension -- so it is a standalone check. Run with: cmake --build . --target check-include-order
find_package(Python3 COMPONENTS Interpreter)
if (Python3_Interpreter_FOUND)
    add_custom_target(check-include-order
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_include_order.py
                ${CMAKE_CURRENT_SOURCE_DIR}/app
                ${CMAKE_CURRENT_SOURCE_DIR}/network
        COMMENT "Checking include order"
        VERBATIM
    )
else ()
    message(STATUS "Python3 not found; target 'check-include-order' unavailable")
endif ()
