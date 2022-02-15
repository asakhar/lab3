cmake_minimum_required(VERSION 3.20)

project(lab3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20) # /std:c++latest for msvc and -std=c++20 for everyone else.
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20")
set(CMAKE_CXX_STANDARD_REQUIRED ON)



add_library(default INTERFACE)
if(MSVC)
  target_compile_options(default INTERFACE /W4 /WX /std:c++latest)
else()
  target_compile_options(default INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()
target_compile_features(default INTERFACE cxx_constexpr cxx_std_23 )

add_executable(lab lab.cpp streams.cpp)
# add_executable(test text.cpp streams.cpp)

target_link_libraries(lab PUBLIC default)
# target_link_libraries(test PUBLIC default)
