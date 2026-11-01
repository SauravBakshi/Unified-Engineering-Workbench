
cmake_minimum_required(VERSION 3.16)

project(UnifiedEngineeringWorkbench VERSION 1.0.0 LANGUAGES CXX)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(UEW_BUILD_TESTS "Build automated tests" ON)

# C++ core library
add_library(uew_core
    src/core/thermal.cpp
    src/core/importers.cpp
    src/core/backprop.cpp
    src/modules/benchmark.cpp
    src/modules/embedded.cpp
)

target_include_directories(uew_core PUBLIC include)

if(MSVC)
    target_compile_options(uew_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(uew_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)
target_link_libraries(uew_core PUBLIC Threads::Threads)

# HTTP server dependency
add_library(httplib INTERFACE)

target_include_directories(httplib INTERFACE
    "${CMAKE_SOURCE_DIR}/third_party"
)

# Main command-line application
add_executable(workbench src/app/main.cpp)
target_link_libraries(workbench PRIVATE uew_core)

# HTTP API server

add_executable(uew_server src/app/server.cpp)

target_link_libraries(uew_server
    PRIVATE
    uew_core
    httplib
)

if(WIN32)
    target_link_libraries(uew_server PRIVATE ws2_32)
    add_executable(Launch WIN32 src/app/launcher.cpp)
    target_link_libraries(Launch PRIVATE shell32)
    if(MINGW)
        foreach(target uew_core workbench uew_server Launch)
            target_link_options(${target} PRIVATE -static -static-libgcc -static-libstdc++)
        endforeach()
    endif()
endif()


# Automated tests
if(UEW_BUILD_TESTS)
    enable_testing()

    add_executable(workbench_tests tests/test_main.cpp)
    target_link_libraries(workbench_tests PRIVATE uew_core)

    add_test(NAME workbench_tests COMMAND workbench_tests)
endif()
