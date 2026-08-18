cmake_minimum_required(VERSION 3.16)
project(bequiet_lightmount_linux CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -Wextra)

add_library(protocol
    src/protocol/crc16.cpp
    src/protocol/report.cpp
)
target_include_directories(protocol PUBLIC src)

add_executable(report_dump src/cli/report_dump.cpp)
target_link_libraries(report_dump PRIVATE protocol)

add_executable(report_send src/cli/report_send.cpp)
target_link_libraries(report_send PRIVATE protocol)

add_executable(report_build src/cli/report_build.cpp)
target_link_libraries(report_build PRIVATE protocol)

enable_testing()
add_executable(test_protocol tests/test_protocol.cpp)
target_link_libraries(test_protocol PRIVATE protocol)
add_test(NAME test_protocol COMMAND test_protocol)
