cmake_minimum_required(VERSION 3.23)

project(CustomCPU LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ..)

include(FetchContent)
 
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v7.0.3  # Replace with a version, tag, or commit hash
)
 
FetchContent_MakeAvailable(ftxui)

add_executable(CustomCPU src/main.cpp src/Processor.cpp)

target_include_directories(CustomCPU PRIVATE include/CustomCPU)

target_compile_options(CustomCPU PRIVATE -Wall)

target_link_libraries(CustomCPU
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)