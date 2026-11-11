#include "app/XeenSetupScreen.h"
#include "formats/xeen/XeenSoundSetup.h"
#include "platform/XeenUserFolders.h"
#include "platform/sdl/SdlWindow.h"
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
#include <optional>
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
        s._focus = p == XeenSetupScreen::Page::Browser ? XeenSetupScreen::browserFocus : 0;
        s._offset = 0;
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
// The main page's heading plate shows "MMModern <version>" and sizes itself to
// it, so its golden digest skips the plate's 25-pixel band; otherwise every
// release would change it.
std::uint64_t pageDigest(IndexedFrame f, XeenSetupScreen::Page page) {
    if (page == XeenSetupScreen::Page::Main)
        std::fill(f.pixels.begin(), f.pixels.begin() + 25 * f.width, std::uint8_t(0));
    return digest(f);
}
// One native-window step: a sender thread finds the setup window and repeats
// its input until the screen answers. A fixed handful of tries gave up while
// the display stalled (WSLg, 2026-11-10), so the step waits up to a minute;
// past that the sender quits the screen and the caller's check fails.
struct NativeStep {
    const std::atomic<bool> &done;
    const std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(60);
    explicit NativeStep(const std::atomic<bool> &finished) : done(finished) {}
    bool waiting() const { return !done && std::chrono::steady_clock::now() < deadline; }
    SDL_Window *setupWindow() const {
        while (waiting()) {
            if (SDL_WasInit(SDL_INIT_VIDEO))
                for (unsigned id = 1; id < 200; ++id)
                    if (auto *w = SDL_GetWindowFromID(id))
                        if (std::string(SDL_GetWindowTitle(w)) == "MMModern setup")
                            return w;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        return nullptr;
    }
};
void nativePlay(XeenSetupScreen &screen, bool mouse) {
    XeenSetupScreenTestAccess::fixture(screen, XeenSetupScreen::Page::Main);
    std::atomic<bool> done{false};
    std::thread sender([&] {
        const NativeStep step(done);
        SDL_Window *const window = step.setupWindow();
        if (window)
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        SDL_Event event{};
        // Early presses can precede the frame that accepts input, so repeat
        // until the screen answers.
        while (window && step.waiting()) {
            if (mouse) {
                int ww = 0, wh = 0, ow = 0, oh = 0;
                SDL_GetWindowSize(window, &ww, &wh);
                auto *renderer = SDL_GetRenderer(window);
                const auto point = renderer && SDL_GetRendererOutputSize(renderer, &ow, &oh) == 0
                    ? frameToWindow(180, 47, ww, wh, ow, oh, 320, 200, PictureAspect::FourThree)
                    : std::nullopt;
                if (point) {
                    event.type = SDL_MOUSEBUTTONDOWN;
                    event.button.button = SDL_BUTTON_LEFT;
                    event.button.x = point->first;
                    event.button.y = point->second;
                    SDL_PushEvent(&event);
                    event.type = SDL_MOUSEBUTTONUP;
                    SDL_PushEvent(&event);
                }
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
#ifndef _WIN32
// M115: the Linux setup screen's controller row changes only the controller
// key of window.ini, written with the other window settings when it closes.
void nativeControllerRow(XeenSetupScreen &screen, const fs::path &scratch) {
    const auto file = scratch / "controller-window.ini";
    WindowPlacement before;before.fullscreen = true;before.width = 800;before.height = 600;before.aspect = PictureAspect::Square;
    check(writeWindowPlacement(file, before), "seed window settings");
    XeenSetupScreenTestAccess::fixture(screen, XeenSetupScreen::Page::Main);
    std::atomic<bool> done{false};
    std::thread sender([&] {
        const NativeStep step(done);
        SDL_Window *const window = step.setupWindow();
        const auto click = [&](int x, int y) {
            int ww = 0, wh = 0, ow = 0, oh = 0;
            SDL_GetWindowSize(window, &ww, &wh);
            auto *renderer = SDL_GetRenderer(window);
            if (!renderer || SDL_GetRendererOutputSize(renderer, &ow, &oh) != 0)
                return;
            const auto point = frameToWindow(x, y, ww, wh, ow, oh, 320, 200, PictureAspect::Square);
            if (!point)
                return;
            SDL_Event event{};
            event.type = SDL_MOUSEBUTTONDOWN;
            event.button.button = SDL_BUTTON_LEFT;
            event.button.windowID = SDL_GetWindowID(window);
            event.button.x = point->first;
            event.button.y = point->second;
            SDL_PushEvent(&event);
            event.type = SDL_MOUSEBUTTONUP;
            SDL_PushEvent(&event);
        };
        // Early clicks can precede the frame that accepts input, as for Play.
        while (window && step.waiting() && !SdlWindow::controllerSetting()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            if (!SdlWindow::controllerSetting()) click(180, 47 + 19 * 4); // Game controller.
        }
        // M116: one click on the Picture filter row selects the first filter.
        while (window && step.waiting() && SdlWindow::pictureFilterSetting() == PictureFilter::Off) {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            if (SdlWindow::pictureFilterSetting() == PictureFilter::Off) click(180, 47 + 19 * 5);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        while (window && step.waiting()) {
            click(180, 47); // Play.
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
        if (!done) {
            SDL_Event event{};
            event.type = SDL_QUIT;
            SDL_PushEvent(&event);
        }
    });
    const auto selected = screen.run(file);
    done = true;
    sender.join();
    check(bool(selected), "Play after the controller row");
    auto expected = before;
    expected.controller = true;
    expected.filter = PictureFilter::AdvMame2x;
    const auto after = readWindowPlacement(file);
    check(after == expected, ("the controller row changed more than the controller key: " + formatWindowPlacement(after)).c_str());
    SdlWindow::useControllerSetting(false);
    SdlWindow::usePictureFilter(PictureFilter::Off);
    SdlWindow::rememberPlacement(std::nullopt);
    std::cout << "Setup screen controller row passed" << std::endl;
}
#endif
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
        // Text input (and the Steam keyboard in Game Mode) starts only on the path line.
        check(s.rows().at(s.focus()).action == "use" && !s.input().textEntry,
              "browser opens on Use this folder without text input");
        s.dispatch(DialogKeyAction{InputKey::Up});
        const auto pathInput = s.input();
        check(pathInput.textEntry, "path line starts text input");
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
        // The main page's buttons are 19 pixels apart, the other pages' rows 15.
        const auto rowY = [](XeenSetupScreen::Page page, int i) {
            return page == XeenSetupScreen::Page::Main ? 50 + 19 * i : 47 + 15 * i;
        };
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
                s.dispatch(*s.input().click(170, rowY(page, int(it - displayed.begin()))));
            }
            std::size_t seen = 0;
            for (int guard = 0; guard < 30; ++guard) {
                const auto displayed = s.displayRows();
                if (page == XeenSetupScreen::Page::Music ||
                    page == XeenSetupScreen::Page::Speech) {
                    bool underSilentHeading = false;
                    for (const auto &row : displayed) {
                        if (row.label == "-- No sound in MMModern yet --") {
                            check(row.action == "noop", "silent heading is actionable");
                            underSilentHeading = true;
                        }
                        if (row.action.compare(0, 7, "device:") == 0) {
                            // All choices after the first heading in the model
                            // are silent; each displayed page repeats it.
                            bool silent = false;
                            for (const auto &modelRow : s.rows()) {
                                if (modelRow.action == "noop")
                                    silent = true;
                                if (modelRow.action == row.action)
                                    break;
                            }
                            check(!silent || underSilentHeading,
                                  "silent device lost its heading on a continuation page");
                        }
                    }
                }
                for (std::size_t i = 0; i < displayed.size(); ++i) {
                    const auto hit = s.input().click(170, rowY(page, int(i)));
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
                        s.dispatch(*s.input().click(170, rowY(page, int(displayed.size() - 1))));
                        check(page == XeenSetupScreen::Page::Browser
                                  ? s.rows()[0].label.find("folder-9") != std::string::npos
                                  : s.page() == XeenSetupScreen::Page::Sound,
                              "last row not activated by click");
                    }
                    break;
                }
                s.dispatch(*s.input().click(170, rowY(page, int(more - displayed.begin()))));
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
                std::cout << p.first << " " << pageDigest(s.frame(), p.second) << '\n';
            }
            return 0;
        }
        #ifdef _WIN32
        const std::uint64_t mainDigest = 7728170877540825945ULL;
#else
        // M115: the controller row. M116: the picture filter row; the page now ends in More.
        const std::uint64_t mainDigest = 11881435348102119593ULL;
#endif
        const std::uint64_t expected[] = {mainDigest, 16505137804929558265ULL,
            8930215273459613646ULL, 12098287078506398707ULL,
            12020560654480869145ULL, 7128448451130753193ULL};
        for (std::size_t i = 0; i < pages.size(); ++i) {
            XeenSetupScreenTestAccess::fixture(s, pages[i].second);
            check(pageDigest(s.frame(), pages[i].second) == expected[i],
                  "setup screen frame changed");
        }
        XeenSetupScreenTestAccess::fixture(s, XeenSetupScreen::Page::Main);
        {
            const auto f = s.frame();
            // Palette index 12 is the heading text color.
            check(std::count(f.pixels.begin(), f.pixels.begin() + 25 * f.width, std::uint8_t(12)) > 0,
                  "main heading has text");
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
        // Down to Copy diagnostics (on Linux the controller row comes before it).
        for (int i = 0; i < 10 && s.rows().at(s.focus()).action != "copy"; ++i)
            s.dispatch(DialogKeyAction{InputKey::Down});
        check(s.rows().at(s.focus()).action == "copy", "Copy diagnostics not reachable with Down");
        s.dispatch(DialogKeyAction{InputKey::Enter});
        char *copied = SDL_GetClipboardText();
        check(copied && std::string(copied).find("game.status:") != std::string::npos &&
                  s.notice() == "Diagnostics copied.",
              "copy diagnostics");
        SDL_free(copied);
#ifndef _WIN32
        {
            // M115: the controller row toggles the window.ini option.
            XeenSetupScreenTestAccess::fixture(s, XeenSetupScreen::Page::Main);
            const auto &rows = s.rows();
            const auto row = std::find_if(rows.begin(), rows.end(), [](const auto &r) { return r.action == "controller"; });
            check(row != rows.end() && row->label == "Game controller: off" && !SdlWindow::controllerSetting(), "controller row, off by default");
            const unsigned hit = 1000 + unsigned(row - rows.begin());
            s.dispatch(DialogKeyAction{hit});
            check(SdlWindow::controllerSetting() && s.rows().at(hit - 1000).label == "Game controller: on", "controller row turns it on");
            s.dispatch(DialogKeyAction{hit});
            check(!SdlWindow::controllerSetting() && s.rows().at(hit - 1000).label == "Game controller: off", "controller row turns it off");
            // M116: the picture filter row cycles F11's list and wraps to off.
            const auto filterRow = std::find_if(rows.begin(), rows.end(), [](const auto &r) { return r.action == "filter"; });
            check(filterRow != rows.end() && filterRow->label == "Picture filter: off" &&
                      SdlWindow::pictureFilterSetting() == PictureFilter::Off,
                  "filter row, off by default");
            const unsigned filterHit = 1000 + unsigned(filterRow - rows.begin());
            s.dispatch(DialogKeyAction{filterHit});
            check(SdlWindow::pictureFilterSetting() == PictureFilter::AdvMame2x &&
                      s.rows().at(filterHit - 1000).label == "Picture filter: AdvMame 2x",
                  "filter row selects the first filter");
            for (int i = 0; i < 11; ++i)
                s.dispatch(DialogKeyAction{filterHit});
            check(SdlWindow::pictureFilterSetting() == PictureFilter::Off &&
                      s.rows().at(filterHit - 1000).label == "Picture filter: off",
                  "filter row wraps to off");
        }
#endif
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        nativePlay(s, false);
        nativePlay(s, true);
#ifndef _WIN32
        nativeControllerRow(s, scratch);
#endif
        std::cout << "Setup screen model passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
