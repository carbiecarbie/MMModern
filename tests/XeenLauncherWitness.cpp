#include "platform/XeenUserFolders.h"
#include "XeenWrapSymbols.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <filesystem>
#include <optional>
#include <cstdlib>
// Exercise the real main/player/audio entry, then fail presentation deterministically.
extern "C" SDL_Renderer * __wrap_SDL_CreateRenderer(SDL_Window *,int,Uint32) {
 SDL_SetError("Launcher test renderer refusal");return nullptr;
}

extern "C" std::optional<std::filesystem::path> wrappedWindowPlacement() asm("__wrap_" XEEN_WRAP_WINDOW_PLACEMENT);
// Keep this process's window settings beside its redirected settings file.
extern "C" std::optional<std::filesystem::path> wrappedWindowPlacement() {
 const auto settings=mmodern::xeenEnvironment("MMODERN_SETTINGS_FILE");
 if(settings.empty())return {};
 return std::filesystem::path(settings).parent_path()/"window.ini";
}
