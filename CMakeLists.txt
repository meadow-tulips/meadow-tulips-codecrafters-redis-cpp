cmake_minimum_required(VERSION 3.13)
project(redis-starter-cpp)
set(CMAKE_CXX_STANDARD 20) # Enable the C++20 standard

set(SOURCE_FILES "src/DataEntity/dataEntity.cpp" "src/Parser/parser.cpp" "src/Server.cpp")

add_executable(server ${SOURCE_FILES})