cmake_minimum_required(VERSION 3.27)
project(
        SimpleSocket
        VERSION 0.5.0
        LANGUAGES C CXX
)

include(cmake/CPM.cmake)

CPMAddPackage("gh:fmtlib/fmt#11.1.4")

set(CMAKE_CXX_STANDARD 20)

option(simple_socket_build_examples "Build server ping pong example" OFF)

message(STATUS "Compiling ${PROJECT_NAME} with version ${PROJECT_VERSION}")


add_library(simpleSocket STATIC
        include/internal/exceptions.hpp
        include/simple_sockets.hpp
        include/internal/unique_value.hpp
        src/socket.cpp
        src/sockets.cpp
)
target_include_directories(simpleSocket PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CMAKE_CROSSCOMPILING)
        message(STATUS "Cross compiling active.")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64|ARM|ARM64)")
            message(STATUS "Target Architecture: ARM")
            find_package(Threads REQUIRED)
            target_link_libraries(simpleSocket PUBLIC fmt::fmt-header-only Threads::Threads)
        elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i686|i386)")
            message(STATUS "Target Architecture: Intel (x86/x64)")
        else()
            message(STATUS "Unknown Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
        endif()
endif()

if(simple_socket_build_examples)
        add_executable(socket_ping_pong example/socket_ping_pong.cpp)
        target_link_libraries(socket_ping_pong PRIVATE simpleSocket)
endif()