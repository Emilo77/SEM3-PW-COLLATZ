cmake_minimum_required(VERSION 3.18)
project(_PW_Collatz)

set(CMAKE_CXX_STANDARD 17)

add_library(teams teams.cpp)
add_executable(_PW_Collatz main.cpp)

target_link_libraries(teams)

