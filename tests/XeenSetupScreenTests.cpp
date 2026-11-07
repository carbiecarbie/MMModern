#include "app/XeenSetupScreen.h"
#include "formats/xeen/XeenSoundSetup.h"
#include "platform/XeenUserFolders.h"
#include "platform/sdl/SdlWindowPlacement.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
using namespace mmodern;
namespace fs = std::filesystem;
namespace mmodern {
struct XeenSetupScreenTestAccess {
    static void fixture(XeenSetupScreen &s, XeenSetupScreen::Page p) {
        s._folder = fs::u8path("/games/world");
        s._directory = fs::u8path("listing");
        s._path = "listing";
        s._hidden = true;
        s._translation = fs::u8path("/translations/russian");
        s._facts = {{"game.status", "usable"},
                    {"sound.music", "Sound Blaster Pro; played"},
                    {"sound.speech", "Sound Blaster 16; played"},
                    {"roms.control", "missing"},
                    {"roms.pcm", "missing"},
                    {"language", "English"},
                    {"saves.used-slots", "0"},
                    {"saves.folder", "/saves/world"}};
        s._sound = {{"music.3.caption", "Sound Blaster Pro; played"},
                    {"speech.1.caption", "Sound Blaster 16; played"},
                    {"music.3.music-port", "0x220,0x240"},
                    {"speech.1.digital-port", "0x220,0x240"},
                    {"speech.1.irq", "5,7"},
                    {"speech.1.dma", "1,3"},
                    {"speech.1.volume", "0,1"}};
        s._answers = {
            {"music", "3"}, {"speech", "1"}, {"music-port", "0x220"}, {"digital-port", "0x220"},
            {"irq", "7"},   {"dma", "1"},    {"volume", "1"}};
        for (bool music : {false, true}) {
            const auto &menu = music ? xeenMusicMenu() : xeenSpeechMenu();
            for (unsigned i = 0; i < menu.size(); ++i) {
                const auto prefix =
                    std::string(music ? "music." : "speech.") + std::to_string(i == 9 ? 0 : i + 1);
                XeenAudioConfig cfg;
                if (music)
                    cfg.music = menu[i].id;
                else
                    cfg.digital = menu[i].id;
                s._sound[prefix + ".caption"] =
                    std::string(menu[i].caption) + "; " + xeenSoundStatus(cfg, music);
                s._sound[prefix + ".id"] = std::to_string(menu[i].id);
            }
        }
        s._page = p;
        s._focus = s._offset = 0;
        s._notice.clear();
        s._error = "Exit code 1\nCannot open game data\nCheck the installation folder.";
        s.refresh();
        if (p == XeenSetupScreen::Page::Music) {
            const auto displayed = s.displayRows();
            for (std::size_t i = 0; i < displayed.size(); ++i)
                if (displayed[i].action == "more")
                    s.dispatch(DialogKeyAction{unsigned(1000 + i)});
        }
    }
    static bool played(const XeenSetupScreen &s) { return s._play; }
    static const std::string &path(const XeenSetupScreen &s) { return s._path; }
};
} // namespace mmodern
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void env(const char *name, const std::string &value) {
#ifdef _WIN32
    _putenv_s(name, value.c_str());
#else
    setenv(name, value.c_str(), 1);
#endif
}
void ppm(const fs::path &path, const IndexedFrame &f) {
    std::ofstream out(path, std::ios::binary);
    out << "P6\n" << f.width << " " << f.height << "\n255\n";
    for (auto i : f.pixels)
        out.write(reinterpret_cast<const char *>(&f.palette[i * 3]), 3);
    if (!out)
        throw std::runtime_error("Cannot write frame");
}
std::uint64_t digest(const IndexedFrame &f) {
    std::uint64_t h = 14695981039346656037ULL;
    for (auto c : f.pixels) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    for (auto c : f.palette) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}
void nativePlay(XeenSetupScreen &screen, bool mouse) {
    XeenSetupScreenTestAccess::fixture(screen, XeenSetupScreen::Page::Main);
    std::atomic<bool> done{false};
    std::thread sender([&] {
        SDL_Window *window = nullptr;
        for (int attempt = 0; attempt < 200 && !done && !window; ++attempt) {
            if (SDL_WasInit(SDL_INIT_VIDEO))
                for (unsigned id = 1; id < 200; ++id)
                    if (auto *w = SDL_GetWindowFromID(id))
                        if (std::string(SDL_GetWindowTitle(w)) == "MMModern setup") {
                            window = w;
                            break;
                        }
            if (!window)
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        if (!window)
            return;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        SDL_Event event{};
        for (int attempt = 0; attempt < 20 && !done; ++attempt) {
            if (mouse) {
                int ww = 0, wh = 0, ow = 0, oh = 0;
                SDL_GetWindowSize(window, &ww, &wh);
                auto *renderer = SDL_GetRenderer(window);
                if (!renderer || SDL_GetRendererOutputSize(renderer, &ow, &oh) != 0)
                    return;
                const auto point =
                    frameToWindow(180, 47, ww, wh, ow, oh, 320, 200, PictureAspect::FourThree);
                if (!point)
                    return;
                event.type = SDL_MOUSEBUTTONDOWN;
                event.button.button = SDL_BUTTON_LEFT;
                event.button.x = point->first;
                event.button.y = point->second;
                SDL_PushEvent(&event);
                event.type = SDL_MOUSEBUTTONUP;
                SDL_PushEvent(&event);
            } else {
                event.type = SDL_KEYDOWN;
                event.key.keysym.sym = SDLK_RETURN;
                event.key.keysym.scancode = SDL_SCANCODE_RETURN;
                SDL_PushEvent(&event);
                event.type = SDL_KEYUP;
                SDL_PushEvent(&event);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
        if (!done) {
            event = {};
            event.type = SDL_QUIT;
            SDL_PushEvent(&event);
        }
    });
    const auto selected = screen.run({});
    done = true;
    sender.join();
    check(selected && *selected == fs::u8path("/games/world"),
          mouse ? "native mouse Play" : "native keyboard Play");
}
int main(int argc, char **argv) {
    try {
        const auto outputDir = argc > 2 ? fs::absolute(fs::u8path(argv[2])) : fs::path{};
        const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto scratch =
            fs::temp_directory_path() / ("mmodern-m114-model-" + std::to_string(tick));
        std::error_code ec;
        fs::create_directories(scratch / "home" / "zeta", ec);
        fs::create_directories(scratch / "home" / "alpha", ec);
        fs::create_directories(scratch / "home" / ".hidden", ec);
        env("MMODERN_PLAYER_DIR", (scratch / "settings").u8string());
        env("HOME", (scratch / "home").u8string());
        env("USER", "player");
        fs::current_path(scratch);
        fs::create_directories("listing/.hidden");
        fs::create_directories("listing/A very long folder name that needs a leading ellipsis");
        for (int i = 0; i < 14; ++i)
            fs::create_directories(fs::path("listing") / ("folder-" + std::to_string(i)));
        XeenSetupScreen s;
        auto listed = XeenSetupScreen::subfolders(scratch / "home", false);
        check(listed.size() >= 2, "folder listing");
        check(listed[0].filename() == "alpha" && listed[1].filename() == "zeta",
              "folder sort or hidden filter");
        listed = XeenSetupScreen::subfolders(scratch / "home", true);
        check(!listed.empty() && listed[0].filename() == ".hidden", "hidden folder toggle");
        std::string error;
        XeenSetupScreen::subfolders(scratch / "missing", false, &error);
        check(!error.empty(), "unreadable folder notice");
        const auto remembered = scratch / "remembered.txt";
        {
            std::ofstream out(remembered, std::ios::binary);
            out << "remembered\n";
        }
        std::vector<fs::path> probed;
        const auto probe = [&](const fs::path &p) -> std::optional<fs::path> {
            probed.push_back(p);
            return p;
        };
        check(XeenSetupScreen::discover(remembered, {scratch / "home"}, probe) ==
                      fs::u8path("remembered") &&
                  probed.size() == 1,
              "remembered folder precedence");
        {
            std::ofstream out(remembered, std::ios::binary | std::ios::trunc);
            out << "invalid\n";
        }
        probed.clear();
        const auto sortedProbe = [&](const fs::path &p) -> std::optional<fs::path> {
            probed.push_back(p);
            return p.filename() == "alpha" ? std::optional<fs::path>{p} : std::nullopt;
        };
        check(XeenSetupScreen::discover(remembered, {scratch / "home"}, sortedProbe) ==
                      scratch / "home" / "alpha" &&
                  probed.size() == 2,
              "sorted candidate precedence");
        check(!XeenSetupScreen::resolve(scratch / "home"), "invalid game folder accepted");
        check(XeenSetupScreen::installRoots(fs::u8path("/home/test")).size() == 3, "install roots");
        check(s.page() == XeenSetupScreen::Page::Browser, "first-run browser");
        const auto pathInput = s.input();
        check(std::find(pathInput.keys.begin(), pathInput.keys.end(), InputKey::Space) ==
                  pathInput.keys.end(),
              "space is intercepted while typing a path");
        s.dispatch(TextInputAction{"/typed"});
        s.dispatch(DialogKeyAction{22}, "/pasted");
        check(s.rows()[0].label.find("/typed/pasted") != std::string::npos,
              "typed and pasted path");
        const auto before = XeenSetupScreenTestAccess::path(s);
        s.dispatch(TextInputAction{"\xc3\xa9"});
        s.dispatch(DialogKeyAction{8});
        check(XeenSetupScreenTestAccess::path(s) == before, "UTF-8 backspace");
        s.dispatch(DialogKeyAction{1});
        s.dispatch(DialogKeyAction{22}, "/path with spaces");
        check(XeenSetupScreenTestAccess::path(s) == "/path with spaces", "select all and paste");
        XeenSetupScreenTestAccess::fixture(s, XeenSetupScreen::Page::Main);
        check(s.focus() == 0, "Play not focused");
        s.dispatch(DialogKeyAction{InputKey::Enter});
        check(XeenSetupScreenTestAccess::played(s), "keyboard Play");
        XeenSetupScreenTestAccess::fixture(s, XeenSetupScreen::Page::Main);
        const auto click = s.input().click(170, 45);
        check(bool(click), "Play mouse hit");
        s.dispatch(*click);
        check(XeenSetupScreenTestAccess::played(s), "mouse Play");
        // Walk every visible window using click actions only. Read-only rows have no hit.
        for (auto page : {XeenSetupScreen::Page::Main, XeenSetupScreen::Page::Browser,
                          XeenSetupScreen::Page::BrowserOptions, XeenSetupScreen::Page::Sound,
                          XeenSetupScreen::Page::Music, XeenSetupScreen::Page::Speech,
                          XeenSetupScreen::Page::Advanced, XeenSetupScreen::Page::Language,
                          XeenSetupScreen::Page::Failure}) {
            XeenSetupScreenTestAccess::fixture(s, page);
            if (page == XeenSetupScreen::Page::Advanced || page == XeenSetupScreen::Page::Failure)
                check(s.rows().at(s.focus()).action != "noop", "initial focus is not actionable");
            if (page == XeenSetupScreen::Page::Failure)
                check(s.rows().at(s.focus()).action == "back", "failure focus must be Back");
            // Start at the first window, even when the initial focus is on a later Back row.
            for (int guard = 0; guard < 30; ++guard) {
                auto displayed = s.displayRows();
                auto it = std::find_if(displayed.begin(), displayed.end(),
                                       [](const auto &r) { return r.action == "previous"; });
                if (it == displayed.end())
                    break;
                s.dispatch(*s.input().click(170, 47 + 15 * int(it - displayed.begin())));
            }
            std::size_t seen = 0;
            for (int guard = 0; guard < 30; ++guard) {
                const auto displayed = s.displayRows();
                for (std::size_t i = 0; i < displayed.size(); ++i) {
                    const auto hit = s.input().click(170, 47 + 15 * int(i));
                    check(bool(hit) == (displayed[i].action != "noop"), "actionable hit coverage");
                    if (displayed[i].action != "previous" && displayed[i].action != "more")
                        ++seen;
                }
                const auto more = std::find_if(displayed.begin(), displayed.end(),
                                               [](const auto &r) { return r.action == "more"; });
                if (more == displayed.end()) {
                    check(seen == s.rows().size(), "click paging missed rows");
                    if (page == XeenSetupScreen::Page::Browser ||
                        page == XeenSetupScreen::Page::Music) {
                        const auto last = displayed.back();
                        s.dispatch(*s.input().click(170, 47 + 15 * int(displayed.size() - 1)));
                        check(page == XeenSetupScreen::Page::Browser
                                  ? s.rows()[0].label.find("folder-9") != std::string::npos
                                  : s.page() == XeenSetupScreen::Page::Sound,
                              "last row not activated by click");
                    }
                    break;
                }
                s.dispatch(*s.input().click(170, 47 + 15 * int(more - displayed.begin())));
                check(guard < 29, "click paging did not terminate");
            }
            for (int i = 0; i < 20; ++i) {
                check(s.focus() >= s.rows().size() || s.rows()[s.focus()].action != "noop",
                      "focus on non-actionable row");
                s.dispatch(DialogKeyAction{InputKey::Down});
            }
        }
        const std::vector<std::pair<const char *, XeenSetupScreen::Page>> pages = {
            {"main", XeenSetupScreen::Page::Main},
            {"sound-advanced", XeenSetupScreen::Page::Advanced},
            {"language", XeenSetupScreen::Page::Language},
            {"browser", XeenSetupScreen::Page::Browser},
            {"failed-start", XeenSetupScreen::Page::Failure},
            {"paged-music", XeenSetupScreen::Page::Music}};
        if (argc > 1 && std::string(argv[1]) == "--frames") {
            const auto dir = outputDir;
            fs::create_directories(dir);
            for (const auto &p : pages) {
                XeenSetupScreenTestAccess::fixture(s, p.second);
                ppm(dir / (std::string(p.first) + ".ppm"), s.frame());
            }
            return 0;
        }
        if (argc > 1 && std::string(argv[1]) == "--digests") {
            for (const auto &p : pages) {
                XeenSetupScreenTestAccess::fixture(s, p.second);
                std::cout << p.first << " " << digest(s.frame()) << '\n';
            }
            return 0;
        }
        #ifdef _WIN32
        const std::uint64_t mainDigest = 10178995649211733790ULL;
#else
        const std::uint64_t mainDigest = 16842633924035143160ULL;
#endif
        const std::uint64_t expected[] = {mainDigest, 1377900911885187413ULL,
            12320302981788104789ULL, 17895145044500846012ULL,
            5305938174462768177ULL, 10117389163922111149ULL};
        for (std::size_t i = 0; i < pages.size(); ++i) {
            XeenSetupScreenTestAccess::fixture(s, pages[i].second);
            check(digest(s.frame()) == expected[i], "setup screen frame changed");
        }
        env("MMODERN_LOG_FILE", (scratch / "failure.log").u8string());
        {
            std::ofstream out(scratch / "failure.log", std::ios::trunc);
            for (int n = 0; n < 15; ++n)
                out << "line " << n << '\n';
        }
        s.failed(7);
        check(s.page() == XeenSetupScreen::Page::Failure, "failed start page");
        check(s.rows()[1].label == "Exit code 7" && s.rows()[2].label == "line 3" &&
                  s.rows()[13].label == "line 14",
              "failed start log tail");
        XeenSetupScreenTestAccess::fixture(s, XeenSetupScreen::Page::Main);
        SDL_SetMainReady();
        check(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == 0, "clipboard SDL setup");
        for (int i = 0; i < 4; ++i)
            s.dispatch(DialogKeyAction{InputKey::Down});
        s.dispatch(DialogKeyAction{InputKey::Enter});
        char *copied = SDL_GetClipboardText();
        check(copied && std::string(copied).find("game.status:") != std::string::npos &&
                  s.notice() == "Diagnostics copied.",
              "copy diagnostics");
        SDL_free(copied);
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        nativePlay(s, false);
        nativePlay(s, true);
        std::cout << "Setup screen model passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
