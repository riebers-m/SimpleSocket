cmake_minimum_required(VERSION 3.27)
project(
        SimpleSocket
        VERSION 0.3.0
        LANGUAGES C CXX
)
add_subdirectory(fmt)

set(CMAKE_CXX_STANDARD 23)
message(STATUS "Compiling ${PROJECT_NAME} with version ${PROJECT_VERSION}")


add_library(simpleSocket STATIC
        include/internal/exceptions.hpp)
target_include_directories(simpleSocket PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if (UNIX)
    target_sources(simpleSocket PRIVATE
            src/linux_socket.cpp
            src/linux_utils.cpp
            src/linux_client_socket.cpp
            src/linux_server_socket.cpp
    )
    message(STATUS "Compiling for linux")

elseif (WIN32)
    message(STATUS "Compiling for windows")
endif ()
target_link_libraries(simpleSocket PUBLIC fmt::fmt-header-only)

add_executable(socket_ping_pong example/socket_ping_pong.cpp)
target_link_libraries(socket_ping_pong PRIVATE simpleSocket)