#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan_demo/glfw.h>

namespace {
    void glfw_error_callback(int error, const char *description) {
        std::cerr << "GLFW error: " << error << " - " << description << "\n";
    }
}

void glfw_init() {
    int major, minor, revision;
    glfwGetVersion(&major, &minor, &revision);
    std::cout << "GLFW: Compiled against " << GLFW_VERSION_MAJOR << "." << GLFW_VERSION_MINOR << "." << GLFW_VERSION_REVISION <<
        ", running against " << major << "." << minor << "." << revision << " - " <<  glfwGetVersionString() << "\n";
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        throw std::runtime_error("GLFW initialization failed");
}

