#include <iostream>

#include <SDL2/SDL.h>

const int WIDTH = 800;
const int HEIGHT = 600;

class SDLApplication {
public:
    void run() {
        init_window();
        main_loop();
    }

    ~SDLApplication() {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

private:
    SDL_Window* window = nullptr;

    void sdlFail() {
        throw std::runtime_error(std::string("SDL: ") + SDL_GetError());
    }

    void init_window() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
            sdlFail();

        window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_ALLOW_HIGHDPI);
        if (!window) {
            sdlFail();
        }
    }

    void main_loop() {
        for (;;) {
            SDL_Event event;
            while (SDL_WaitEvent(&event)) {
                switch (event.type) {
                case SDL_QUIT:
                    return;
                }
            }
            sdlFail();
        }
    }
};

int main(int argc, char **argv) {
    SDLApplication app;

    try {
        app.run();
    } catch (const std::runtime_error& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        throw;
    } catch (...) {
        throw;
    }

    return 0;
}
