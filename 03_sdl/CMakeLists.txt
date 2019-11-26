cmake_minimum_required(VERSION 3.8)
project(SDL_demo CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)

add_executable(main src/main.cc)
target_link_libraries(main SDL2::SDL2)
