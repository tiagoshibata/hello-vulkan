#include "sdl_window.hh"

#include <stdexcept>

#include <SDL2/SDL_vulkan.h>

#include "quit_exception.hh"

const int WIDTH = 800;
const int HEIGHT = 600;

namespace {
void sdl_fail() {
    throw std::runtime_error(std::string("SDL: ") + SDL_GetError());
}
}

SDLWindow::SDLWindow() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        sdl_fail();

    window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        sdl_fail();
    }
}

std::vector<const char*> SDLWindow::get_vulkan_extensions() const {
    unsigned int count;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr))
        sdl_fail();

    std::vector<const char*> extensions(count);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &count, extensions.data()))
        sdl_fail();
    return extensions;
}

VkSurfaceKHR SDLWindow::create_vulkan_surface(const VkInstance& instance) {
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(window, instance, &surface))
        sdl_fail();
    return surface;
}

std::pair<int, int> SDLWindow::get_drawable_size() const {
    int width, height;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);
    return {width, height};
}

#include <iostream>

void SDLWindow::wait_window_show_event() {
    SDL_Event event;
    // while (SDL_WaitEvent(&event)) {
    while (SDL_WaitEventTimeout(&event, 500)) {
        switch (event.type) {
        case SDL_QUIT:
            throw quit_exception();
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_EXPOSED:
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                std::cout << "SDL_WINDOWEVENT\n";
                return;
            }
        }
    }
    sdl_fail();
}

bool SDLWindow::pool() {
    bool needs_redraw = false;
    SDL_Event event;
    SDL_WaitEvent(&event);
    do {
        switch (event.type) {
        case SDL_QUIT:
            throw quit_exception();
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_EXPOSED:
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                needs_redraw = true;
            }
        }
    } while (SDL_PollEvent(&event));
    std::cout << "Event - needs_redraw = " << needs_redraw << "\n";
    return true;
    return needs_redraw;
}

SDLWindow::~SDLWindow() {
    SDL_DestroyWindow(window);
    SDL_Quit();
}
