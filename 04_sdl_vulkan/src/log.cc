#ifdef REDIRECT_STD_STREAMS_TO_SDL
/*
https://wiki.libsdl.org/SDL_Log
http://www.cplusplus.com/reference/iostream/clog/
http://www.cplusplus.com/reference/ios/ios/rdbuf/
http://www.cplusplus.com/reference/streambuf/streambuf/
https://stackoverflow.com/questions/14086417/how-to-write-custom-input-stream-in-c
https://stackoverflow.com/questions/23726848/inherit-from-stdostringstream
https://stackoverflow.com/questions/27368897/simplest-way-to-create-class-that-behaves-like-stringstream?lq=1
https://github.com/tcbrindle/sdlxx/blob/master/include/sdl%2B%2B/log.hpp
*/

#include <SDL2/SDL.h>

class SdlLogRedirect {
public:
    SdlLogRedirect() {

    }

    ~SdlLogRedirect() {

    }

private:

};

class BlahStream {
    void coisa() {
        SDL_Log("%s", "test");
    }
};

// static SdlLogRedirect sdl_log_redirect;

#endif
