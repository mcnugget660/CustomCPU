cmake_minimum_required(VERSION 3.23)

project(Assembler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../..)

add_executable(Assembler src/main.cpp)

target_include_directories(Assembler PRIVATE include)