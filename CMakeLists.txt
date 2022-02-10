cmake_minimum_required(VERSION 3.12)

project(lab3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

add_library(default INTERFACE)
if(MSVC)
  target_compile_options(default INTERFACE /W4 /WX)
else()
  target_compile_options(default INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(lab lab.cpp file.cpp)
add_executable(test text.cpp)

target_link_libraries(lab INTERFACE default)
target_link_libraries(test INTERFACE default)
