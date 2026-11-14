#include "platform/sdl/SdlCrtFilter.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mmodern;

namespace {
void require(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(std::string(message) + ": " + SDL_GetError());
}
int exercise(SDL_Renderer *renderer, int argc, char **argv) {
    constexpr int w = 320, h = 200, ow = 1280, oh = 800;
    std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> input(nullptr, SDL_FreeSurface);
    if (argc == 3) {
        input.reset(SDL_LoadBMP(argv[1]));
        require(input && input->w == w && input->h == h, "Expected a native 320x200 BMP");
    }
    std::vector<Uint32> source(w*h, 0xff808080);
    for (int y = 0; y < h/2; ++y) std::fill_n(source.begin() + y*w, w/2, 0xff0090c0);
    if (input) require(SDL_ConvertPixels(w,h,input->format->format,input->pixels,input->pitch,
                          SDL_PIXELFORMAT_ARGB8888,source.data(),w*4) == 0,"Convert frame");
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture(
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w,h), SDL_DestroyTexture);
    require(texture && SDL_UpdateTexture(texture.get(),nullptr,source.data(),w*4)==0,"Upload CRT source");
    SDL_SetTextureScaleMode(texture.get(),SDL_ScaleModeLinear);
    SdlCrtFilter crt;
    for (auto aspect : {PictureAspect::FourThree, PictureAspect::Square}) {
        const auto rect = pictureRect(ow,oh,w,h,aspect);
        int previousContrast = 0;
        for (auto profile : {PictureFilter::Off, PictureFilter::CrtVga, PictureFilter::CrtPhosphor}) {
            SDL_SetRenderDrawColor(renderer,0,0,0,255);
            SDL_RenderClear(renderer);
            if (profile == PictureFilter::Off) {
                SDL_Rect dest{rect.x,rect.y,rect.width,rect.height};
                SDL_SetTextureScaleMode(texture.get(),SDL_ScaleModeNearest);
                require(SDL_RenderCopy(renderer,texture.get(),nullptr,&dest)==0,"Draw reference");
            } else {
                SDL_SetTextureScaleMode(texture.get(),SDL_ScaleModeLinear);
                if (!crt.draw(renderer,texture.get(),w,h,rect,oh,profile)) throw std::runtime_error(crt.error());
            }
            std::vector<Uint32> output(ow*oh);
            require(SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_ARGB8888,output.data(),ow*4)==0,"Read CRT pixels");
            if (input) {
                const auto name = std::string(argv[2]) + "-" + std::string(pictureFilterName(profile)) +
                                  (aspect == PictureAspect::Square ? "-square.bmp" : "-4x3.bmp");
                std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)> surface(
                    SDL_CreateRGBSurfaceWithFormatFrom(output.data(),ow,oh,32,ow*4,SDL_PIXELFORMAT_ARGB8888),SDL_FreeSurface);
                require(surface && SDL_SaveBMP(surface.get(),name.c_str())==0,"Save CRT readback");
            } else {
                if (rect.x > 0) require((output[oh/2*ow+rect.x-1]&0xffffffu)==0,"Black side border");
                const auto sky = output[oh/4*ow+rect.x+rect.width/4];
                require((sky&255)>100 && ((sky>>16)&255)<8,"Top-left quadrant retains its orientation and colour");
                int low=255, high=0;
                for (int x=ow/2+40; x<ow/2+56; ++x) {
                    int value = output[(oh*3/4)*ow+x]&255;
                    low=std::min(low,value); high=std::max(high,value);
                }
                require(low>45 && high<195,"Flat grey remains readable");
                if (profile != PictureFilter::Off) require(high-low>previousContrast+2,"Increasing phosphor contrast");
                if (profile != PictureFilter::Off) {
                    const int x = ow/2+40, y = oh*3/4;
                    const int upper = output[y*ow+x]&255, lower = output[(y+1)*ow+x]&255;
                    require(std::abs(upper-lower)>4,"VGA scanlines remain visible at native Deck resolution");
                }
                previousContrast=high-low;
                // SDL draws the original pointer and F11 label after direct GL.
                // A subsequent SDL texture copy must use its own shader and viewport.
                SDL_Rect sourceRect{0,0,1,1}, marker{rect.x+12,rect.y+12,8,8};
                require(SDL_RenderCopy(renderer,texture.get(),&sourceRect,&marker)==0,"Draw SDL overlay after CRT");
                Uint32 sample = 0;
                SDL_Rect pixel{marker.x+4,marker.y+4,1,1};
                require(SDL_RenderReadPixels(renderer,&pixel,SDL_PIXELFORMAT_ARGB8888,&sample,4)==0 &&
                        (sample&0xffffffu)==(source[0]&0xffffffu),"SDL overlay retains its original pixels");
            }
            SDL_RenderPresent(renderer);
        }
    }
    return 0;
}
}

int main(int argc,char **argv) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO)!=0) return 77;
    struct Quit { ~Quit(){SDL_Quit();} } quit;
    std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window(
        SDL_CreateWindow("MMModern CRT readback",0,0,1280,800,SDL_WINDOW_HIDDEN|SDL_WINDOW_OPENGL),SDL_DestroyWindow);
    if (!window) { std::cout << "No desktop OpenGL window: " << SDL_GetError() << '\n'; return 77; }
    SDL_SetHint(SDL_HINT_RENDER_DRIVER,"opengl");
    std::unique_ptr<SDL_Renderer,decltype(&SDL_DestroyRenderer)> renderer(
        SDL_CreateRenderer(window.get(),-1,SDL_RENDERER_ACCELERATED),SDL_DestroyRenderer);
    SDL_RendererInfo info{};
    if (!renderer || SDL_GetRendererInfo(renderer.get(),&info) || std::string(info.name)!="opengl") {
        std::cout << "No desktop OpenGL renderer\n"; return 77;
    }
    try { return exercise(renderer.get(),argc,argv); }
    catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
