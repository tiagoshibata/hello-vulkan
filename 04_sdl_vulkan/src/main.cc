#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "sdl_window.hh"
#include "vulkan.hh"

class App {
private:
    SDLWindow window_;
    Vulkan vulkan_;

public:
    App() : vulkan_(window_.get_vulkan_extensions()) {}

    void run() {
        const auto surface = window_.create_vulkan_surface(vulkan_.get_instance());
        const auto extent = window_.get_drawable_size();
        vulkan_.initialize(surface, extent.first, extent.second);
        main_loop();
    }

    void main_loop() {
        for (;;) {
        // while (window_.pool()) {
            if (window_.pool())
                step();
        }
    }

    void step() {
        vulkan_.draw_frame();
    }
};

int main(int, char **) {
    try {
        App app;
        app.run();
    } catch (const std::runtime_error& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        // throw;
    } catch (...) {
        throw;
    }

    return 0;
}
