cmake_minimum_required(VERSION 3.5.0)

PROJECT(Lab_4_5)

set (CMAKE_CXX_STANDARD 20)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
    # set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
endif()

ADD_EXECUTABLE(test serial_port.hpp http_server.hpp general_utils.hpp general_utils.cpp main.cpp)
add_executable(general serial_port.hpp temperature_logger.cpp)

IF (WIN32)
    TARGET_LINK_LIBRARIES(test ws2_32)
ENDIF(WIN32)


