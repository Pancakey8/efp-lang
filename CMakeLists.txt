cmake_minimum_required(VERSION 3.16)
project(efp LANGUAGES CXX)

option(DEBUG_MODE off)

file(GLOB src src/*.cpp)
add_executable(efp "${src}")
target_include_directories(efp PRIVATE include/)
target_compile_features(efp PRIVATE cxx_std_23)
target_compile_options(efp PRIVATE -Wall -Wextra -Wpedantic)
if(DEBUG_MODE)
    target_link_options(efp PRIVATE -fsanitize=address,undefined)
endif()
