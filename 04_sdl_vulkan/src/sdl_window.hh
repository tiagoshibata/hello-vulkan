#ifndef SDL_WINDOW_HH_
#define SDL_WINDOW_HH_

#include <vector>

#include <SDL2/SDL.h>
#include <vulkan/vulkan.h>

class SDLWindow {
public:
    SDLWindow();
    SDLWindow(const SDLWindow&) = delete;
    SDLWindow& operator=(const SDLWindow&) = delete;
    [[nodiscard]] std::vector<const char*> get_vulkan_extensions() const;
    [[nodiscard]] VkSurfaceKHR create_vulkan_surface(const VkInstance& instance);
    [[nodiscard]] std::pair<int, int> get_drawable_size() const;
    bool pool();
    ~SDLWindow();

private:
    SDL_Window* window = nullptr;
};

#endif
