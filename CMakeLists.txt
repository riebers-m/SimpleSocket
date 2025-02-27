cmake_minimum_required(VERSION 3.27)
project(
        SimpleSocket
        VERSION 0.5.0
        LANGUAGES C CXX
)
#add_subdirectory(fmt)

include(cmake/CPM.cmake)

CPMAddPackage("gh:fmtlib/fmt#11.1.4")

set(CMAKE_CXX_STANDARD 20)
message(STATUS "Compiling ${PROJECT_NAME} with version ${PROJECT_VERSION}")


add_library(simpleSocket STATIC
        include/internal/exceptions.hpp
        include/simple_sockets.hpp
        include/internal/unique_value.hpp
        src/socket.cpp
        src/sockets.cpp
)
target_include_directories(simpleSocket PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(simpleSocket PUBLIC fmt::fmt-header-only)

add_executable(socket_ping_pong example/socket_ping_pong.cpp)
target_link_libraries(socket_ping_pong PRIVATE simpleSocket)