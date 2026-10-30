#ifndef MMODERN_TESTS_SDL_MOUSE_TEST_SUPPORT_H
#define MMODERN_TESTS_SDL_MOUSE_TEST_SUPPORT_H

#include "platform/sdl/SdlWindowPlacement.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdexcept>

namespace sdl_mouse_test {
// Test processes have one production SDL window at a time.
inline SDL_Window *window() {
    for (Uint32 id = 1; id < 65536; ++id)
        if (auto *w = SDL_GetWindowFromID(id); w && SDL_GetRenderer(w)) return w;
    throw std::runtime_error("Test SDL window not found");
}
inline void position(SDL_MouseButtonEvent &event, int x, int y,
        mmodern::PictureAspect aspect = mmodern::PictureAspect::FourThree) {
    auto *w = window(); auto *renderer = SDL_GetRenderer(w);
    int ww, wh, ow, oh; SDL_GetWindowSize(w, &ww, &wh);
    if (SDL_GetRendererOutputSize(renderer, &ow, &oh) != 0)
        throw std::runtime_error("Test SDL output size unavailable");
    auto p = mmodern::frameToWindow(x, y, ww, wh, ow, oh, 320, 200, aspect);
    if (!p && (x == -1 || x == 320) && y >= 0 && y < 200) {
        const auto edge = mmodern::frameToWindow(x < 0 ? 0 : 319, y, ww, wh, ow, oh, 320, 200, aspect);
        if (edge) {
            int wx = edge->first;
            do { wx += x < 0 ? -1 : 1; }
            while (wx >= 0 && wx < ww && mmodern::windowToFrame(wx, edge->second, ww, wh, ow, oh, 320, 200, aspect));
            p = std::pair<int,int>{wx, edge->second};
        }
    }
    if (!p) throw std::runtime_error("Test frame pixel has no window coordinate");
    event.windowID = SDL_GetWindowID(w); event.x = p->first; event.y = p->second;
}
}
#endif
