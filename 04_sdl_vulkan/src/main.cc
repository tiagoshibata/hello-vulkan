#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "quit_exception.hh"
#include "sdl_window.hh"
#include "vulkan.hh"

class App {
private:
    SDLWindow window_;
    Vulkan vulkan_;

public:
    App() : vulkan_(window_.get_vulkan_extensions(), [this]() {return window_.get_drawable_size();}, SDLWindow::wait_window_show_event) {}

    void run() {
        const auto surface = window_.create_vulkan_surface(vulkan_.get_instance());
        vulkan_.initialize(surface);
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
    } catch (const quit_exception& e) {

    } catch (const std::runtime_error& e) {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        throw;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        throw;
    } catch (...) {
        throw;
    }

    return 0;
}
