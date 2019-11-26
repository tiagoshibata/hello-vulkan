cmake_minimum_required (VERSION 3.8)
project (Vulkan CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
add_executable (main src/vulkan_demo/main.cpp)
target_link_libraries (main Vulkan::Vulkan glfw)

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "*.frag"
    "*.vert"
)
set(GLSLANGVALIDATOR "glslangValidator")
foreach(GLSL ${GLSL_SOURCE_FILES})
    # get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${GLSL}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSLANGVALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)
add_dependencies(main Shaders)
