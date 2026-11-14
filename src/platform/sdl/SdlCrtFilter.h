#pragma once

#include "platform/sdl/SdlWindowPlacement.h"
#include <memory>
#include <string>

struct SDL_Renderer;
struct SDL_Texture;

namespace mmodern {

// One GL program per SDL renderer. Constructed lazily on the first CRT frame.
class SdlCrtFilter {
public:
    SdlCrtFilter();
    ~SdlCrtFilter();
    bool draw(SDL_Renderer *renderer, SDL_Texture *frame, int width, int height,
              PictureRect picture, int outputHeight, PictureFilter filter);
    const std::string &error() const { return _error; }
    void reset();
private:
    struct Program;
    std::unique_ptr<Program> _program;
    std::string _error;
};

} // namespace mmodern
