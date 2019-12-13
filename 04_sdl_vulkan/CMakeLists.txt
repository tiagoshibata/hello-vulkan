cmake_minimum_required (VERSION 3.8)
project (Vulkan CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

file(GLOB main_src src/*.cc)
add_executable(main ${main_src})
target_link_libraries(main Vulkan::Vulkan SDL2::SDL2)
if(MSVC)
    target_compile_options(main PRIVATE /W4 /WX)
else()
    target_compile_options(main PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "src/*.frag"
    "src/*.vert"
)

# TODO consider using shaderc, spirv-opt
# https://gitlab.freedesktop.org/monado/monado/blob/e86320cd65db20aaed4b3b797e005f6001a2a5d6/cmake/SPIR-V.cmake
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