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
