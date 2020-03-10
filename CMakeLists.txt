cmake_minimum_required(VERSION 3.15)
project(chip8)
set(CMAKE_CXX_STANDARD 20)

INCLUDE(FindPkgConfig)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

pkg_search_module(SDL2IMAGE REQUIRED SDL2_image)
include_directories(${SDL2IMAGE_INCLUDE_DIRS})

add_executable(chip8 main.cpp sdl_helpers.cpp sdl_helpers.h chip8.c chip8.hpp)
target_link_libraries(chip8 PRIVATE SDL2::SDL2 ${SDL2IMAGE_LIBRARIES})
