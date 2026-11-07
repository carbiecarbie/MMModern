#define main mmodern_test_unused_main
#include "../src/main.cpp"
#undef main
#include "platform/XeenWonStore.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <array>
#include <atomic>
#include <chrono>
#include <fstream>
#include <stdexcept>
#include <thread>
namespace fs = std::filesystem;
static fs::path placementFile;
extern "C" std::optional<fs::path> __wrap__ZN7mmodern19windowPlacementFileB5cxx11Ev() {
    return placementFile;
}
static void variable(const char *name, const std::string &value) {
#ifdef _WIN32
    _putenv_s(name, value.c_str());
#else
    setenv(name, value.c_str(), 1);
#endif
}
static void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
static void choose(mmodern::XeenSetupScreen &screen, const std::string &action) {
    const auto &rows = screen.rows();
    const auto found = std::find_if(rows.begin(), rows.end(),
                                    [&](const auto &row) { return row.action == action; });
    require(found != rows.end(), "setup action unavailable");
    const auto target = std::size_t(found - rows.begin());
    for (int guard = 0; screen.focus() != target && guard < 100; ++guard) {
        if (screen.focus() >= screen.rows().size())
            screen.dispatch(mmodern::DialogKeyAction{mmodern::InputKey::Enter});
        else
            screen.dispatch(mmodern::DialogKeyAction{mmodern::InputKey::Down});
    }
    require(screen.focus() == target, "cannot focus setup action");
    screen.dispatch(mmodern::DialogKeyAction{mmodern::InputKey::Enter});
}
int main(int argc, char **argv) {
    try {
        require(argc == 4, "Need original game, scratch directory and optional translation");
        const auto game = fs::absolute(fs::u8path(argv[1]));
        const auto scratch = fs::absolute(fs::u8path(argv[2]));
        const bool russian = *argv[3] != '\0';
        fs::create_directories(scratch);
        placementFile = scratch / "window.ini";
        variable("MMODERN_PLAYER_DIR", scratch.u8string());
        variable("MMODERN_LOG_FILE", (scratch / "mmodern.log").u8string());
        variable("MMODERN_SETTINGS_FILE", (scratch / "settings.ini").u8string());
        variable("SDL_AUDIODRIVER", "dummy");
        {
            std::ofstream f(scratch / "game-folder.txt", std::ios::binary | std::ios::trunc);
            f << game.u8string() << '\n';
            require(bool(f), "remembered folder");
        }
        // An unavailable EXE digital driver skips the startup intro, leaving the
        // DAT title as the first game window. This also identifies the chosen CFG.
        std::array<char, 29> cfg{};
        cfg[0] = 9;
        cfg[2] = 0x20;
        cfg[3] = 2;
        cfg[4] = 3;
        cfg[6] = 0x20;
        cfg[7] = 2;
        cfg[8] = 7;
        cfg[20] = 1;
        cfg[24] = 1;
        {
            std::ofstream f(scratch / "XEEN.CFG", std::ios::binary | std::ios::trunc);
            f.write(cfg.data(), cfg.size());
            require(bool(f), "owned CFG");
        }
        require(mmodern::xeenWriteLanguage(scratch / "settings.ini", mmodern::XeenLanguage::English,
                                           {}),
                "language setting");
        {
            fs::create_directory(scratch / "game-folder.txt.tmp");
            {
                mmodern::XeenSetupScreen unwritable;
                require(!unwritable.notice().empty() &&
                            unwritable.page() == mmodern::XeenSetupScreen::Page::Main,
                        "remember failure ended setup");
            }
            fs::remove(scratch / "game-folder.txt.tmp");
            mmodern::XeenSetupScreen screen;
            require(screen.folder() == game, "remembered game folder");
            const auto world = mmodern::XeenSetupScreen::resolve(game / "WORLD");
            require(world && *world == game, "WORLD resolves to parent");
            const auto worldWithSlash = mmodern::XeenSetupScreen::resolve(game / "WORLD" / "");
            require(worldWithSlash && *worldWithSlash == game, "WORLD with trailing separator");
            choose(screen, "sound");
            choose(screen, "music");
            require(screen.rows()[screen.focus()].action == "device:5",
                    "music list did not select current device");
            choose(screen, "device:8");
            require(screen.notice().find("MT-32") != std::string::npos, "MT-32 ROM hint missing");
#ifndef _WIN32
            require(std::any_of(screen.rows().begin(), screen.rows().end(),
                                [](const auto &row) { return row.action == "roms"; }),
                    "MT-32 ROM folder action missing");
#endif
            choose(screen, "music");
            choose(screen, "device:5");
            choose(screen, "speech");
            require(screen.rows()[screen.focus()].action == "device:8",
                    "speech list did not select current device");
            choose(screen, "device:9");
            choose(screen, "advanced");
            choose(screen, "back-sound");
            choose(screen, "save-sound");
            const auto report = mmodern::xeenDiagnostics(game);
            require(report.find("sound.speech: Sound Blaster 16") != std::string::npos,
                    "sound setup did not write chosen device");
            choose(screen, "sound");
            choose(screen, "reset");
            require(!fs::exists(scratch / "XEEN.CFG"), "reset did not restore installed CFG");
            {
                std::ofstream f(scratch / "XEEN.CFG", std::ios::binary | std::ios::trunc);
                f.write(cfg.data(), cfg.size());
                require(bool(f), "restore chosen CFG");
            }
            choose(screen, "language");
            choose(screen, "toggle-language");
            choose(screen, "save-language");
            require(screen.page() == mmodern::XeenSetupScreen::Page::Language &&
                        !screen.notice().empty(),
                    "invalid translation accepted");
            require(!mmodern::xeenPlayerTranslation(), "refused translation changed language");
            if (russian) {
                choose(screen, "translation");
                // The browser opens on "Use this folder"; the path line is above it.
                screen.dispatch(mmodern::DialogKeyAction{mmodern::InputKey::Up});
                screen.dispatch(mmodern::DialogKeyAction{1});
                screen.dispatch(mmodern::DialogKeyAction{22}, argv[3]);
                choose(screen, "use");
                choose(screen, "save-language");
                require(screen.page() == mmodern::XeenSetupScreen::Page::Main &&
                            bool(mmodern::xeenPlayerTranslation()),
                        "Russian translation selection failed");
            }
        }
        std::atomic<bool> done{false}, title{false};
        std::thread sender([&] {
            for (int i = 0; i < 1200 && !done; ++i) {
                if (SDL_WasInit(SDL_INIT_VIDEO))
                    for (unsigned id = 1; id < 200; ++id)
                        if (auto *w = SDL_GetWindowFromID(id)) {
                            const std::string name = SDL_GetWindowTitle(w);
                            if (name == "MMModern setup") {
                                SDL_Event e{};
                                e.type = SDL_KEYDOWN;
                                e.key.keysym.sym = SDLK_RETURN;
                                e.key.keysym.scancode = SDL_SCANCODE_RETURN;
                                SDL_PushEvent(&e);
                                e.type = SDL_KEYUP;
                                SDL_PushEvent(&e);
                            } else if (name == "MMModern") {
                                title = true;
                                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                                SDL_Event e{};
                                e.type = SDL_QUIT;
                                SDL_PushEvent(&e);
                                return;
                            }
                        }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            if (!done) {
                SDL_Event e{};
                e.type = SDL_QUIT;
                SDL_PushEvent(&e);
            }
        });
        const int status = runWithArguments({"mmodern"});
        done = true;
        sender.join();
        require(status == 0, "no-argument player entry failed");
        require(title, "original title not reached");
        std::ifstream in(scratch / "mmodern.log");
        const std::string log((std::istreambuf_iterator<char>(in)), {});
        require(log.find("cfg.source: owned") != std::string::npos, "owned CFG not selected");
        require(log.find("sound.speech: Covox Sound Master 2") != std::string::npos,
                "chosen digital device not selected");
        require(log.find(russian ? "language: Russian" : "language: English") != std::string::npos,
                "chosen language not selected");
        std::cout << "Remembered folder reached original title with owned CFG and language\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
