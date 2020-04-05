cmake_minimum_required(VERSION 3.16)
project(Vulkan CXX)

# Properties for all targets
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CheckIPOSupported)
check_ipo_supported()
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

find_program(GLSLANGVALIDATOR glslangValidator)
if(NOT GLSLANGVALIDATOR)
	message(FATAL_ERROR "glslangValidator not found")
endif()
find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

file(GLOB main_sources CONFIGURE_DEPENDS src/*.cc)
add_executable(main ${main_sources})
target_link_libraries(main Vulkan::Vulkan SDL2::SDL2)
if(MSVC)
    target_compile_options(main PRIVATE /W4 /WX)
else()
    target_compile_options(main PRIVATE -Wall -Wextra -Werror -pedantic)
endif()


function(vulkan_shader input output)
    get_filename_component(variable_name ${input} NAME)
    string(MAKE_C_IDENTIFIER ${variable_name} variable_name)
    set(variable_name "${variable_name}_spirv")
    # TODO enable optimizations in Release builds (consider using shaderc or spirv-opt)
    add_custom_command(
        OUTPUT ${output}
        COMMAND ${GLSLANGVALIDATOR} -V110 ${input} --vn ${variable_name} -o ${output}
        DEPENDS ${input})
endfunction()

file(GLOB glsl_sources CONFIGURE_DEPENDS "src/shaders/*")
foreach(source ${glsl_sources})
    get_filename_component(output ${source} NAME)
    set(output "${output}.h")
    vulkan_shader(${source} ${output})
    list(APPEND spirv_shaders ${output})
endforeach()

add_custom_target(shaders DEPENDS ${spirv_shaders})
add_dependencies(main shaders)
