cmake_minimum_required (VERSION 2.6)
project (Vulkan CXX)
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
include_directories(inc)
add_executable (main src/vulkan_demo/main.cpp src/vulkan_demo/glfw.cpp)
target_link_libraries (main ${Vulkan_LIBRARIES} glfw)
