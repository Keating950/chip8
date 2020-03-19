cmake_minimum_required(VERSION 3.15)
project(chip8)
set(C_STANDARD 18)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c18")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "/home/bigfoot-lives/cProjects/cppProjects/chip8")

INCLUDE(FindPkgConfig)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

pkg_search_module(SDL2IMAGE REQUIRED SDL2_image)
include_directories(${SDL2IMAGE_INCLUDE_DIRS})

add_executable(chip8 src/main.c src/chip8.c lib/chip8.h src/av_io.c lib/av_io.h)
target_link_libraries(chip8 PRIVATE SDL2::SDL2 ${SDL2IMAGE_LIBRARIES})
