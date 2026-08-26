cmake_minimum_required(VERSION 3.23)

project(CPU LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

include(FetchContent)
 
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v7.0.3  # Replace with a version, tag, or commit hash
)
 
FetchContent_MakeAvailable(ftxui)

add_executable(CPU src/main.cpp src/Processor.cpp src/Menu.cpp)

target_include_directories(CPU PRIVATE include)

target_compile_options(CPU PRIVATE -Wall)

target_link_libraries(CPU PRIVATE ftxui)