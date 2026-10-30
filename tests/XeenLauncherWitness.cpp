#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <filesystem>
#include <optional>
#include <cstdlib>
// Exercise the real main/player/audio entry, then fail presentation deterministically.
extern "C" SDL_Renderer * __wrap_SDL_CreateRenderer(SDL_Window *,int,Uint32) {
 SDL_SetError("Launcher test renderer refusal");return nullptr;
}

// Keep this process's window settings beside its redirected settings file.
extern "C" std::optional<std::filesystem::path> __wrap__ZN7mmodern19windowPlacementFileEv() {
 const auto *settings=_wgetenv(L"MMODERN_SETTINGS_FILE");
 if(!settings)return {};
 return std::filesystem::path(settings).parent_path()/"window.ini";
}
