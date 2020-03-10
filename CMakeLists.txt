cmake_minimum_required(VERSION 3.15)
project(chip8)
set(CMAKE_CXX_STANDARD 20)

if(APPLE)
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
elseif(UNIX)
    include_directories(/usr/include/SDL2)
endif()



add_executable(chip8 main.cpp)
target_link_libraries(chip8 ${SDL2_LIBRARIES})
