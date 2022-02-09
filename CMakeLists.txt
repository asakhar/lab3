cmake_minimum_required(VERSION 3.12)

project(lab3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(lab lab.cpp file.cpp)

if(MSVC)
  target_compile_options(lab PRIVATE /W4 /WX)
else()
  target_compile_options(lab PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()