#define FORBIDDEN_SYMBOL_ALLOW_ALL
#include "app/XeenSetupScreen.h"
#include "MMModernVersion.h"
#include "app/XeenPlayerSupport.h"
#include "core/PlayerAction.h"
#include "graphics/fonts/bdf.h"
#include "graphics/surface.h"
#include "platform/XeenFileOperations.h"
#include "platform/XeenUserFolders.h"
#include "platform/sdl/SdlWindow.h"
#include "platform/sdl/SdlWindowPlacement.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <thread>
#ifndef _WIN32
#include <cerrno>
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;
#endif

namespace Graphics {
FORWARD_DECLARE_FONT(g_sysfont);
FORWARD_DECLARE_FONT(g_sysfont_big);
}
namespace mmodern {
namespace {
using Path = std::filesystem::path;
constexpr std::size_t previousRow = std::size_t(-2);
constexpr std::size_t moreRow = std::size_t(-1);
constexpr std::size_t pageSize = 7;
// Row geometry: the main page shows large buttons on the right, the other
// pages a full-width list. frame() and input() share it.
struct RowBox {
    int x, y, w, h;
};
RowBox rowBox(bool main, int i) {
    return main ? RowBox{164, 43 + i * 19, 146, 17} : RowBox{8, 41 + i * 15, 304, 14};
}
// Stone and gold palette, drawn without any original game resource: the
// setup screen runs before the game folder is known.
enum SetupColor : std::uint8_t {
    Stone,
    Stone2,
    Stone3,
    Stone4,
    MortarDark,
    MortarLight,
    Outline,
    GoldLight,
    Gold,
    GoldDark,
    GoldDeep,
    PlateFill,
    TitleText,
    Label,
    Value,
    PanelFill,
    PanelShadow,
    PanelLight,
    ButtonFill,
    ButtonLight,
    ButtonDark,
    SelectedFill,
    SelectedText,
    Warning,
    Dim,
    ColorCount
};
constexpr std::uint8_t setupPalette[ColorCount][3] = {
    {30, 26, 21},   {35, 30, 24},   {25, 21, 17},   {41, 35, 28},   {15, 13, 11},
    {44, 38, 30},   {12, 10, 8},    {242, 210, 122}, {216, 170, 72}, {168, 122, 40},
    {106, 74, 24},  {58, 44, 26},   {240, 208, 112}, {168, 148, 112}, {236, 228, 208},
    {22, 18, 14},   {10, 8, 7},     {74, 64, 54},   {58, 51, 43},   {98, 88, 72},
    {26, 22, 18},   {90, 67, 32},   {255, 240, 176}, {255, 200, 94}, {122, 106, 78}};
std::map<std::string, std::string> facts(const std::string &text) {
    std::map<std::string, std::string> result;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        const auto p = line.find(": ");
        if (p != std::string::npos)
            result[line.substr(0, p)] = line.substr(p + 2);
    }
    return result;
}
std::string shortened(std::string text, std::size_t limit = 48) {
    if (text.size() <= limit)
        return text;
    auto p = text.size() - (limit - 3);
    while (p < text.size() && (static_cast<unsigned char>(text[p]) & 0xc0) == 0x80)
        ++p;
    return "..." + text.substr(p);
}
std::string expand(const std::string &value) {
    auto text = value;
    auto home = xeenEnvironment("HOME").u8string();
#ifdef _WIN32
    home = xeenEnvironment("USERPROFILE").u8string();
    const auto local = xeenEnvironment("LOCALAPPDATA").u8string();
    if (text.rfind("%LOCALAPPDATA%", 0) == 0)
        text = local + text.substr(14);
    if (text.rfind("%USERPROFILE%", 0) == 0)
        text = home + text.substr(13);
#else
    if (text == "~")
        text = home;
    else if (text.rfind("~/", 0) == 0)
        text = home + text.substr(1);
    const auto xdg = xeenEnvironment("XDG_DATA_HOME").u8string();
    if (text.rfind("$XDG_DATA_HOME", 0) == 0)
        text = xdg + text.substr(14);
#endif
    return text;
}
void openFolder(const Path &path) {
#ifndef _WIN32
    auto name = path.u8string();
    char executable[] = "xdg-open";
    char *args[] = {executable, name.data(), nullptr};
    pid_t pid{};
    const int error = posix_spawnp(&pid, executable, nullptr, nullptr, args, environ);
    if (error)
        throw std::runtime_error("Could not open folder (error " + std::to_string(error) + ")");
    std::thread([pid] {
        int status = 0;
        while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
        }
    }).detach();
#else
    (void)path;
#endif
}
std::string title(XeenSetupScreen::Page page) {
    using P = XeenSetupScreen::Page;
    switch (page) {
    case P::Main:
        return "MMModern " MMODERN_VERSION;
    case P::Browser:
        return "Choose a folder";
    case P::BrowserOptions:
        return "Locations and options";
    case P::Sound:
        return "Sound setup";
    case P::Music:
        return "Music and sound effects";
    case P::Speech:
        return "Speech and digital sounds";
    case P::Advanced:
        return "Advanced hardware settings";
    case P::Language:
        return "Language";
    case P::Failure:
        return "MMModern could not start";
    }
    return {};
}
} // namespace
std::vector<XeenSetupScreen::Path> XeenSetupScreen::installRoots(const Path &home) {
    // Heroic:
    // https://github.com/Heroic-Games-Launcher/HeroicGamesLauncher/wiki/Removing-Uninstalling-Heroic-Games-Launcher
    // GOG Linux: https://support.gog.com/hc/en-us/articles/213039625-Where-is-my-game-installed
    // Wine prefix: https://manpages.debian.org/stretch/wine/wine.1.en.html
    // Wine drive_c: https://docs.huihoo.com/wine/wine-user-guide.pdf
    // GOG Windows:
    // https://support.gog.com/hc/en-us/articles/360013424697-How-to-set-Launch-parameters-for-game
    return {home / "Games" / "Heroic", home / "GOG Games",
            home / ".wine" / "drive_c" / "GOG Games"};
}
std::vector<XeenSetupScreen::Path> XeenSetupScreen::subfolders(const Path &path, bool hidden,
                                                               std::string *error) {
    std::vector<Path> result;
    std::error_code ec;
    std::filesystem::directory_iterator i(path, ec), end;
    if (ec) {
        if (error)
            *error = "Cannot read folder: " + path.u8string();
        return result;
    }
    for (; i != end; i.increment(ec)) {
        if (ec) {
            if (error)
                *error = "Cannot read folder: " + path.u8string();
            break;
        }
        std::error_code stat;
        if (!i->is_directory(stat))
            continue;
        const auto name = i->path().filename().u8string();
        if (!hidden && !name.empty() && name[0] == '.')
            continue;
        result.push_back(i->path());
    }
    if (ec && error)
        *error = "Cannot read folder: " + path.u8string();
    std::sort(result.begin(), result.end(), [](const Path &a, const Path &b) {
        return a.filename().u8string() < b.filename().u8string();
    });
    return result;
}
std::optional<XeenSetupScreen::Path> XeenSetupScreen::resolve(const Path &candidate) {
    if (candidate.empty())
        return {};
    const auto usable = [](const Path &p) {
        try {
            return facts(xeenDiagnostics(p))["game.status"] == "usable";
        } catch (...) {
            return false;
        }
    };
    auto path = candidate;
    if (path.filename().empty() && path != path.root_path())
        path = path.parent_path();
    auto leaf = path.filename().u8string();
    std::transform(leaf.begin(), leaf.end(), leaf.begin(),
                   [](unsigned char c) { return char(std::toupper(c)); });
    if (leaf == "WORLD")
        return usable(path.parent_path()) ? std::optional<Path>{path.parent_path()} : std::nullopt;
    return usable(path) ? std::optional<Path>{path} : std::nullopt;
}
std::optional<XeenSetupScreen::Path>
XeenSetupScreen::discover(const Path &saved, const std::vector<Path> &roots,
                          const std::function<std::optional<Path>(const Path &)> &probe) {
    std::ifstream input(saved, std::ios::binary);
    std::string line;
    std::optional<Path> remembered;
    if (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.rfind("\xef\xbb\xbf", 0) == 0)
            line.erase(0, 3);
        if (!line.empty())
            remembered = probe(std::filesystem::u8path(line));
    }
    input.close();
    if (remembered)
        return remembered;
    for (const auto &root : roots)
        for (const auto &p : subfolders(root, false))
            if (auto found = probe(p))
                return found;
    return {};
}
XeenSetupScreen::XeenSetupScreen() {
    if (auto found = discover(xeenPlayerDirectory() / "game-folder.txt",
                              installRoots(xeenEnvironment("HOME")), resolve))
        _folder = *found;
    if (_folder.empty())
        browser(xeenEnvironment("HOME"));
    else {
        try {
            saveFolder(_folder);
        } catch (const std::exception &e) {
            _notice = e.what();
        }
        report();
        refresh();
    }
}
void XeenSetupScreen::saveFolder(const Path &path) {
    const auto folder = xeenPlayerDirectory();
    std::filesystem::create_directories(folder);
    const auto target = folder / "game-folder.txt", temporary = folder / "game-folder.txt.tmp";
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out || !(out << path.u8string() << '\n') || !out.flush())
            throw std::runtime_error("Cannot remember game folder");
    }
    if (!xeenReplaceSmallFile(temporary, target)) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw std::runtime_error("Cannot remember game folder");
    }
}
void XeenSetupScreen::report() {
    _report = xeenDiagnostics(_folder);
    _facts = facts(_report);
}
void XeenSetupScreen::browser(const Path &path, bool translation) {
    _directory = path.empty() ? std::filesystem::current_path() : path;
    _path = _directory.u8string();
    _selectPath = false;
    _translationBrowser = translation;
    _page = Page::Browser;
    _focus = browserFocus;
    _offset = 0;
    _notice.clear();
    refresh();
}
void XeenSetupScreen::command(const std::vector<std::string> &arguments) {
    std::ostringstream out;
    auto *saved = std::cout.rdbuf(out.rdbuf());
    try {
        if (arguments.size() > 1 && arguments[1] == "--sound-setup")
            xeenSoundSetupCommand(arguments);
        else
            xeenLanguageSetupCommand(arguments);
        std::cout.rdbuf(saved);
    } catch (...) {
        std::cout.rdbuf(saved);
        throw;
    }
    _sound = facts(out.str());
}
void XeenSetupScreen::showSound() {
    try {
        command({"mmodern", "--sound-setup", _folder.u8string()});
        _answers.clear();
        for (const auto &field :
             {"music", "speech", "digital-port", "music-port", "irq", "dma", "volume"})
            _answers[field] = _sound[std::string("current.") + field];
        _page = Page::Sound;
        _focus = _offset = 0;
        soundHint("music");
        refresh();
    } catch (const std::exception &e) {
        _notice = std::string("Sound setup: ") + e.what() + " Reset available.";
        _page = Page::Sound;
        _sound.clear();
        _focus = _offset = 0;
        refresh();
    }
}
void XeenSetupScreen::soundHint(const std::string &kind) {
    const auto prefix = kind + "." + _answers[kind];
    const auto caption = _sound[prefix + ".caption"];
    if (caption.rfind("None;", 0) == 0)
        _notice = kind == "music" ? "No music or sound effects." : "No speech or digital sounds.";
    else if (caption.find("; silent") != std::string::npos)
        _notice = "MMModern does not play this device yet.";
    else if (kind == "music" && _sound[prefix + ".id"] == "4") {
        const bool found = _facts["roms.control"].rfind("identified", 0) == 0 &&
                           _facts["roms.pcm"].rfind("identified", 0) == 0;
        _notice = found ? "Your MT-32 ROMs were found." : "MT-32 needs control 1.07 and PCM ROMs.";
    } else if (_sound[prefix + ".id"] == (kind == "music" ? "3" : "10"))
        _notice = "The device the CD edition installs.";
    else
        _notice.clear();
}
void XeenSetupScreen::refresh() {
    _rows.clear();
    const auto add = [&](std::string label, std::string action) {
        _rows.push_back({std::move(label), std::move(action)});
    };
    const auto soundLabel = [&](const char *kind) {
        auto value = _sound[std::string(kind) + "." + _answers[kind] + ".caption"];
        const auto end = value.find(';');
        return value.substr(0, end);
    };
    switch (_page) {
    case Page::Main:
        add("Play", "play");
        add("Change game folder", "browser");
        add("Sound setup", "sound");
        add("Language", "language");
#ifndef _WIN32
        // M115: the window.ini option; the Windows launcher has its own box.
        add(SdlWindow::controllerSetting() ? "Game controller: on" : "Game controller: off", "controller");
#endif
        add("Copy diagnostics", "copy");
#ifndef _WIN32
        add("Open saves folder", "saves");
        add("Open log folder", "log");
#endif
        break;
    case Page::Browser: {
        add(std::string(_selectPath ? "Path (all): " : "Path: ") +
                shortened(_path, _selectPath ? 36 : 42),
            "path");
        add("Use this folder", "use");
        add("Parent", "parent");
        add("Locations and options", "locations");
        std::string error;
        for (const auto &p : subfolders(_directory, _hidden, &error))
            add(shortened(p.filename().u8string(), 46), "dir:" + p.u8string());
        if (!error.empty())
            _notice = error;
        break;
    }
    case Page::BrowserOptions:
        add("Home", "home");
#ifndef _WIN32
        add("/media/" + xeenEnvironment("USER").u8string(), "media");
        add("/run/media/" + xeenEnvironment("USER").u8string(), "runmedia");
#endif
        add(_hidden ? "Hide hidden folders" : "Show hidden folders", "hidden");
        add("Back", "browser-back");
        break;
    case Page::Sound:
        if (_sound.empty()) {
            add("Reset to installed", "reset");
            add("Back", "back");
            break;
        }
        add("Music: " + shortened(soundLabel("music"), 38), "music");
        add("Speech: " + shortened(soundLabel("speech"), 37), "speech");
        {
            bool questions = false;
            for (const auto &kind : {"speech", "music"})
                for (const auto &field : {"digital-port", "irq", "dma", "volume", "music-port"})
                    if (!_sound[std::string(kind) + "." + _answers[kind] + "." + field].empty())
                        questions = true;
            if (questions)
                add("Advanced hardware settings", "advanced");
        }
#ifndef _WIN32
        if (_sound["music." + _answers["music"] + ".id"] == "4")
            add("Open ROM folder", "roms");
#endif
        add("Reset to installed", "reset");
        add("Save", "save-sound");
        add("Cancel", "back");
        break;
    case Page::Music:
    case Page::Speech: {
        const auto kind = _page == Page::Music ? "music" : "speech";
        const auto recommended = _page == Page::Music ? "3" : "10";
        std::vector<Row> played, none, silent;
        for (const auto &key : {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"}) {
            const auto prefix = std::string(kind) + "." + key;
            auto caption = _sound[prefix + ".caption"];
            const auto split = caption.find("; ");
            const auto status =
                split == std::string::npos ? std::string{} : caption.substr(split + 2);
            caption = caption.substr(0, split);
            if (_sound[prefix + ".id"] == recommended)
                caption += " (recommended)";
            else if (_page == Page::Music && _sound[prefix + ".id"] == "4")
                caption += " (needs ROMs)";
            Row row{shortened(caption, 47), std::string("device:") + key};
            if (caption == "None")
                none.push_back(row);
            else if (status == "silent")
                silent.push_back(row);
            else
                played.push_back(row);
        }
        _rows = played;
        _rows.insert(_rows.end(), none.begin(), none.end());
        if (!silent.empty())
            add("-- No sound in MMModern yet --", "noop");
        for (const auto &row : silent) {
            // Each page must give the silent choices their own context. Keep
            // the repeated heading read-only, just like the first one.
            if (_rows.size() % pageSize == 0)
                add("-- No sound in MMModern yet --", "noop");
            _rows.push_back(row);
        }
        add("Back", "back-sound");
        break;
    }
    case Page::Advanced:
        add("Original card settings; output quality unchanged", "noop");
        for (const auto &kind : {"speech", "music"})
            for (const auto &field : {"digital-port", "irq", "dma", "volume", "music-port"}) {
                const auto key = std::string(kind) + "." + _answers[kind] + "." + field;
                if (_sound[key].empty())
                    continue;
                const std::string label = std::string(field) == "digital-port" ? "Speech card port"
                                          : std::string(field) == "music-port" ? "Music card port"
                                          : std::string(field) == "irq"        ? "IRQ"
                                          : std::string(field) == "dma"
                                              ? "DMA"
                                              : "Adjust Sound Blaster volumes";
                add(label + ": " +
                        (_answers[field] == "1" && std::string(field) == "volume" ? "Yes"
                         : _answers[field] == "0" && std::string(field) == "volume"
                             ? "No"
                             : _answers[field]),
                    std::string("field:") + key);
            }
        add("Back", "back-sound");
        break;
    case Page::Language:
        add(std::string("Game language: ") + (_russian ? "Russian" : "English"), "toggle-language");
        add("Translation: " + shortened(_translation.u8string(), 35), "translation");
        add("Save", "save-language");
        add("Cancel", "back");
        break;
    case Page::Failure:
        add("The game exited with an error.", "noop");
        {
            std::istringstream in(_error);
            std::string line;
            while (std::getline(in, line))
                add(shortened(line, 48), "noop");
        }
        add("Back", "back");
        break;
    }
    if (_rows.empty()) {
        _focus = 0;
        _offset = 0;
        return;
    }
    if (_focus != previousRow && _focus != moreRow) {
        if (_focus >= _rows.size() || _rows[_focus].action == "noop") {
            const auto it = std::find_if(_rows.begin(), _rows.end(),
                                         [](const Row &r) { return r.action != "noop"; });
            _focus = std::size_t(it - _rows.begin());
        }
        _offset = _rows.size() > 9 ? (_focus / pageSize) * pageSize : 0;
    }
}
std::vector<std::size_t> XeenSetupScreen::visibleIndices() const {
    std::vector<std::size_t> result;
    const auto count = _rows.size() > 9 ? pageSize : 9;
    if (_offset > 0)
        result.push_back(previousRow);
    for (std::size_t i = _offset; i < _rows.size() && i < _offset + count; ++i)
        result.push_back(i);
    if (_offset + count < _rows.size())
        result.push_back(moreRow);
    return result;
}
XeenSetupScreen::Row XeenSetupScreen::rowAt(std::size_t index) const {
    if (index == previousRow)
        return {"Previous", "previous"};
    if (index == moreRow)
        return {"More", "more"};
    return _rows.at(index);
}
std::vector<XeenSetupScreen::Row> XeenSetupScreen::displayRows() const {
    std::vector<Row> result;
    for (auto index : visibleIndices())
        result.push_back(rowAt(index));
    return result;
}
void XeenSetupScreen::move(int amount) {
    auto visible = visibleIndices();
    visible.erase(std::remove_if(visible.begin(), visible.end(),
                                 [&](auto i) { return rowAt(i).action == "noop"; }),
                  visible.end());
    if (visible.empty())
        return;
    auto it = std::find(visible.begin(), visible.end(), _focus);
    const auto position = it == visible.end() ? 0 : int(it - visible.begin());
    _focus = visible[(position + amount + int(visible.size())) % visible.size()];
    refresh();
}
void XeenSetupScreen::activate(std::string action) {
    try {
        if (action == "noop")
            return;
        if (action == "previous" || action == "more") {
            _offset = action == "more" ? _offset + pageSize : _offset - pageSize;
            _focus = (_offset + pageSize < _rows.size()) ? moreRow : previousRow;
            for (auto index : visibleIndices())
                if (index < _rows.size() && _rows[index].action != "noop") {
                    _focus = index;
                    break;
                }
            refresh();
            return;
        }
        if (action == "locations") {
            _page = Page::BrowserOptions;
            _focus = _offset = 0;
            refresh();
            return;
        }
        if (action == "browser-back") {
            _page = Page::Browser;
            _focus = browserFocus;
            _offset = 0;
            refresh();
            return;
        }
        if (action == "path") {
            browser(std::filesystem::u8path(_path), _translationBrowser);
            return;
        }
        if (action == "play") {
            _play = true;
            return;
        }
        if (action == "back") {
            _page = Page::Main;
            _focus = _offset = 0;
            _notice.clear();
            if (!_folder.empty())
                report();
        } else if (action == "back-sound") {
            _page = Page::Sound;
            _focus = _offset = 0;
        } else if (action == "browser") {
            browser(_folder.empty() ? xeenEnvironment("HOME") : _folder);
            return;
        } else if (action == "sound") {
            showSound();
            return;
        } else if (action == "language") {
            _page = Page::Language;
            _russian = _facts["language"] == "Russian" || !_facts["language.error"].empty();
            _translation = std::filesystem::u8path(expand(_facts["language.translation"]));
            _focus = _offset = 0;
        } else if (action == "copy") {
            report();
            _notice = SDL_SetClipboardText(_report.c_str()) == 0
                          ? "Diagnostics copied."
                          : std::string("Copy failed: ") + SDL_GetError();
        } else if (action == "saves" || action == "log" || action == "roms") {
            Path p = action == "log"
                         ? xeenLogFile().parent_path()
                         : std::filesystem::u8path(
                               expand(_facts[action == "saves" ? "saves.folder" : "roms.folder"]));
            if (action == "roms")
                std::filesystem::create_directories(p);
            if (p.empty() || !std::filesystem::is_directory(p))
                _notice = "Folder is created when first needed.";
            else
                openFolder(p);
        } else if (action == "parent") {
            browser(_directory.parent_path(), _translationBrowser);
            return;
        } else if (action == "home") {
            browser(xeenEnvironment("HOME"), _translationBrowser);
            return;
        } else if (action == "media" || action == "runmedia") {
            browser(Path(action == "media" ? "/media" : "/run/media") / xeenEnvironment("USER"),
                    _translationBrowser);
            return;
        } else if (action == "hidden") {
            _hidden = !_hidden;
            _notice.clear();
            _page = Page::Browser;
            _focus = browserFocus;
            _offset = 0;
        } else if (action.rfind("dir:", 0) == 0) {
            browser(std::filesystem::u8path(action.substr(4)), _translationBrowser);
            return;
        } else if (action == "use") {
            const auto candidate = std::filesystem::u8path(_path);
            if (_translationBrowser) {
                _translation = candidate;
                _page = Page::Language;
                _focus = _offset = 0;
            } else if (auto p = resolve(candidate)) {
                saveFolder(*p);
                _folder = *p;
                report();
                _page = Page::Main;
                _focus = _offset = 0;
                _notice.clear();
            } else
                _notice = "No usable World of Xeen CD data found.";
        } else if (action == "music" || action == "speech") {
            _page = action == "music" ? Page::Music : Page::Speech;
            _focus = _offset = 0;
            refresh();
            const auto chosen = "device:" + _answers[action];
            const auto it = std::find_if(_rows.begin(), _rows.end(),
                                         [&](const Row &row) { return row.action == chosen; });
            if (it != _rows.end())
                _focus = std::size_t(it - _rows.begin());
        } else if (action == "advanced") {
            _page = Page::Advanced;
            _focus = _offset = 0;
        } else if (action.rfind("device:", 0) == 0) {
            const auto kind = _page == Page::Music ? "music" : "speech";
            _answers[kind] = action.substr(7);
            const auto prefix = std::string(kind) + "." + _answers[kind];
            const auto caption = _sound[prefix + ".caption"];
            for (const auto &field : {"digital-port", "music-port", "irq", "dma", "volume"}) {
                const auto values = _sound[prefix + "." + field];
                if (values.empty())
                    continue;
                std::istringstream in(values);
                std::string value;
                bool found = false;
                while (std::getline(in, value, ','))
                    if (value == _answers[field])
                        found = true;
                if (!found)
                    _answers[field] = values.substr(0, values.find(','));
            }
            (void)caption;
            soundHint(kind);
            _page = Page::Sound;
            _focus = _offset = 0;
        } else if (action.rfind("field:", 0) == 0) {
            const auto key = action.substr(6);
            const auto field = key.substr(key.rfind('.') + 1);
            const auto list = _sound[key];
            std::vector<std::string> vals;
            std::istringstream in(list);
            std::string value;
            while (std::getline(in, value, ','))
                vals.push_back(value);
            auto it = std::find(vals.begin(), vals.end(), _answers[field]);
            _answers[field] = it == vals.end() || ++it == vals.end() ? vals.front() : *it;
        } else if (action == "reset") {
            command({"mmodern", "--sound-setup", _folder.u8string(), "--reset"});
            report();
            _page = Page::Main;
            _focus = _offset = 0;
            _notice = "Sound reset to installed.";
        } else if (action == "save-sound") {
            std::vector<std::string> args = {
                "mmodern",          "--sound-setup", _folder.u8string(), "--speech",
                _answers["speech"], "--music",       _answers["music"]};
            for (const auto &kind : {"speech", "music"})
                for (const auto &field : {"digital-port", "irq", "dma", "volume", "music-port"}) {
                    const auto key = std::string(kind) + "." + _answers[kind] + "." + field;
                    if (!_sound[key].empty()) {
                        const auto option = std::string("--") + field;
                        if (std::find(args.begin(), args.end(), option) == args.end()) {
                            args.push_back(option);
                            args.push_back(_answers[field]);
                        }
                    }
                }
            command(args);
            report();
            _page = Page::Main;
            _focus = _offset = 0;
            _notice = "Sound setup saved.";
        } else if (action == "controller") {
            // Kept in the loaded window settings and written with them when
            // this window closes; the game's windows read it when they open.
            SdlWindow::useControllerSetting(!SdlWindow::controllerSetting());
        } else if (action == "toggle-language")
            _russian = !_russian;
        else if (action == "translation") {
            browser(_translation.empty() ? xeenEnvironment("HOME") : _translation, true);
            return;
        } else if (action == "save-language") {
            std::vector<std::string> args = {"mmodern", "--language-setup", "--language",
                                             _russian ? "russian" : "english"};
            if (!_translation.empty()) {
                args.push_back("--translation");
                args.push_back(_translation.u8string());
            }
            command(args);
            report();
            _page = Page::Main;
            _focus = _offset = 0;
            _notice = "Language saved.";
        }
    } catch (const std::exception &e) {
        _notice = e.what();
    }
    refresh();
}
void XeenSetupScreen::dispatch(const PlayerAction &action, const std::string &clipboard) {
    if (const auto *text = std::get_if<TextInputAction>(&action)) {
        if (_page == Page::Browser && !_rows.empty() && _focus < _rows.size() &&
            _rows[_focus].action == "path") {
            if (_selectPath)
                _path.clear();
            _selectPath = false;
            _path += text->text;
            refresh();
        }
        return;
    }
    const auto *key = std::get_if<DialogKeyAction>(&action);
    if (!key)
        return;
    const auto k = key->key;
    if (k != 1 && k != 8 && k != 22)
        ++_contextId;
    if (k == InputKey::Escape) {
        if (_page == Page::Main ||
            (_page == Page::Browser && _folder.empty() && !_translationBrowser))
            _quit = true;
        else if (_page == Page::BrowserOptions) {
            activate("browser-back");
        } else if (_page == Page::Music || _page == Page::Speech || _page == Page::Advanced)
            activate("back-sound");
        else if (_page == Page::Browser && _translationBrowser) {
            _page = Page::Language;
            _focus = _offset = 0;
            refresh();
        } else
            activate("back");
        return;
    }
    if (k == 9 || k == InputKey::Down || k == InputKey::Right) {
        move(1);
        return;
    }
    if (k == InputKey::Up || k == InputKey::Left) {
        move(-1);
        return;
    }
    if (k == 1 && _page == Page::Browser && !_rows.empty() && _focus < _rows.size() &&
        _rows[_focus].action == "path") {
        _selectPath = true;
        refresh();
        return;
    }
    if (k == 8 && _page == Page::Browser && !_rows.empty() && _focus < _rows.size() &&
        _rows[_focus].action == "path") {
        if (_selectPath)
            _path.clear();
        else if (!_path.empty()) {
            auto pos = _path.size() - 1;
            while (pos > 0 && (static_cast<unsigned char>(_path[pos]) & 0xc0) == 0x80)
                --pos;
            _path.erase(pos);
        }
        _selectPath = false;
        refresh();
        return;
    }
    if (k == 22 && _page == Page::Browser && !_rows.empty() && _focus < _rows.size() &&
        _rows[_focus].action == "path") {
        if (_selectPath)
            _path.clear();
        _selectPath = false;
        _path += clipboard;
        refresh();
        return;
    }
    if (k >= 1000 && k < 1009) {
        const auto visible = visibleIndices();
        const auto slot = k - 1000;
        if (slot < visible.size()) {
            const auto index = visible[slot];
            if (rowAt(index).action != "noop") {
                _focus = index;
                activate(rowAt(index).action);
            }
        }
        return;
    }
    if ((k == InputKey::Enter || k == InputKey::Space) && !_rows.empty())
        activate(rowAt(_focus).action);
}
DialogInput XeenSetupScreen::input() const {
    DialogInput input;
    input.keys = {1,
                  9,
                  8,
                  22,
                  InputKey::Enter,
                  InputKey::Escape,
                  InputKey::Up,
                  InputKey::Down,
                  InputKey::Left,
                  InputKey::Right};
    input.textEntry = _page == Page::Browser && !_rows.empty() && _focus < _rows.size() &&
                      _rows[_focus].action == "path";
    input.freeText = input.textEntry;
    if (!input.textEntry)
        input.keys.push_back(InputKey::Space);
    for (unsigned i = 0; i < visibleIndices().size(); ++i)
        if (rowAt(visibleIndices()[i]).action != "noop") {
            const auto box = rowBox(_page == Page::Main, int(i));
            input.hits.push_back({box.x, box.y, box.x + box.w, box.y + box.h, 1000 + i, {}});
        }
    return input;
}
IndexedFrame XeenSetupScreen::frame() const {
    IndexedFrame f;
    f.width = 320;
    f.height = 200;
    f.pixels.assign(320 * 200, Stone);
    for (int c = 0; c < ColorCount; ++c)
        for (int k = 0; k < 3; ++k)
            f.palette[c * 3 + k] = setupPalette[c][k];
    const auto put = [&](int x, int y, int color) {
        if (x >= 0 && y >= 0 && x < 320 && y < 200)
            f.pixels[y * 320 + x] = std::uint8_t(color);
    };
    const auto rect = [&](int x, int y, int w, int h, int color) {
        for (int yy = std::max(0, y); yy < std::min(200, y + h); ++yy)
            for (int xx = std::max(0, x); xx < std::min(320, x + w); ++xx)
                f.pixels[yy * 320 + xx] = std::uint8_t(color);
    };
    // A box lit from the top left: raised with a light edge there, sunken with a dark one.
    const auto edged = [&](int x, int y, int w, int h, int fill, int light, int dark) {
        rect(x, y, w, h, dark);
        rect(x, y, w - 1, h - 1, light);
        rect(x + 1, y + 1, w - 2, h - 2, fill);
    };
    const auto bevel = [&](int x, int y, int w, int h, int fill, int light, int dark) {
        rect(x - 1, y - 1, w + 2, h + 2, Outline);
        edged(x, y, w, h, fill, light, dark);
    };
    const auto diamond = [&](int cx, int cy, int r, int color) {
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = std::abs(dy) - r; dx <= r - std::abs(dy); ++dx)
                put(cx + dx, cy + dy, color);
    };
    // Stone wall: staggered 40x20 blocks with a fixed speckle.
    for (int y = 0; y < 200; ++y)
        for (int x = 0; x < 320; ++x) {
            std::uint32_t h = std::uint32_t(x) * 374761393u ^ std::uint32_t(y) * 668265263u;
            h = (h ^ (h >> 13)) * 1274126177u;
            h ^= h >> 16;
            const auto speckle = h % 9;
            int color = speckle < 5 ? Stone : speckle < 7 ? Stone2 : speckle < 8 ? Stone3 : Stone4;
            const int bx = (x + (y / 20 % 2 ? 20 : 0)) % 40, by = y % 20;
            if (bx == 0 || by == 0)
                color = MortarDark;
            else if (bx == 1 || by == 1)
                color = MortarLight;
            f.pixels[y * 320 + x] = std::uint8_t(color);
        }
    const int ring[] = {Outline, GoldLight, Gold, GoldDark, Outline};
    for (int i = 0; i < 5; ++i) {
        rect(i, i, 320 - 2 * i, 1, ring[i]);
        rect(i, 199 - i, 320 - 2 * i, 1, ring[i]);
        rect(i, i, 1, 200 - 2 * i, ring[i]);
        rect(319 - i, i, 1, 200 - 2 * i, ring[i]);
    }
    const int corners[4][2] = {{3, 3}, {316, 3}, {3, 196}, {316, 196}};
    for (const auto &c : corners) {
        diamond(c[0], c[1], 5, Outline);
        diamond(c[0], c[1], 4, Gold);
        diamond(c[0], c[1], 2, GoldDeep);
    }
    if (!Graphics::g_sysfont)
        Graphics::create_g_sysfont();
    if (!Graphics::g_sysfont_big)
        Graphics::create_g_sysfont_big();
    const Graphics::Font &small = *Graphics::g_sysfont, &bold = *Graphics::g_sysfont_big;
    Graphics::Surface surface;
    surface.init(320, 200, 320, f.pixels.data(), Graphics::PixelFormat::createFormatCLUT8());
    const auto codepoints = [](const std::string &s) {
        std::vector<std::uint32_t> result;
        for (std::size_t p = 0; p < s.size();) {
            unsigned char c = s[p++];
            std::uint32_t cp = c;
            if (c >= 0xc2 && c <= 0xf4) {
                unsigned count = c < 0xe0 ? 1 : c < 0xf0 ? 2 : 3;
                cp = c & ((1u << (6 - count)) - 1);
                while (count-- && p < s.size())
                    cp = (cp << 6) | (static_cast<unsigned char>(s[p++]) & 63);
            }
            result.push_back(cp >= 383 ? 0 : cp);
        }
        return result;
    };
    const auto width = [&](const Graphics::Font &font, const std::string &s, int spacing = 0) {
        int w = 0;
        for (auto cp : codepoints(s))
            w += font.getCharWidth(cp) + spacing;
        return w;
    };
    // Shortens from the left, as shortened() does, until the text fits the width.
    const auto fit = [&](const Graphics::Font &font, std::string s, int limit) {
        if (width(font, s) <= limit)
            return s;
        while (!s.empty() && width(font, "..." + s) > limit) {
            std::size_t n = 1;
            while (n < s.size() && (static_cast<unsigned char>(s[n]) & 0xc0) == 0x80)
                ++n;
            s.erase(0, n);
        }
        return "..." + s;
    };
    const auto text = [&](const Graphics::Font &font, int x, int y, const std::string &s,
                          int color, int spacing = 0) {
        for (auto cp : codepoints(s)) {
            const int w = font.getCharWidth(cp) + spacing;
            if (x + w > 316)
                break;
            font.drawChar(&surface, cp, x, y, color);
            x += w;
        }
    };
    const auto shadowed = [&](const Graphics::Font &font, int x, int y, const std::string &s,
                              int color, int spacing = 0) {
        text(font, x + 1, y + 1, s, Outline, spacing);
        text(font, x, y, s, color, spacing);
    };
    const auto centered = [&](const Graphics::Font &font, int y, const std::string &s, int color) {
        const auto shown = fit(font, s, 300);
        text(font, 160 - width(font, shown) / 2, y, shown, color);
    };
    // One pixel of tracking keeps "rn" in "MMModern" from reading as "m".
    const auto heading = title(_page);
    const int plate = width(bold, heading, 1) + 23;
    bevel(160 - plate / 2, 7, plate, 17, PlateFill, GoldLight, GoldDeep);
    shadowed(bold, 160 - plate / 2 + 12, 9, heading, TitleText, 1);
    if (_page == Page::Main)
        centered(small, 27, _folder.u8string(), Label);
    else if (_page == Page::Browser)
        centered(small, 27, _directory.u8string(), Label);
    else if (_page == Page::Sound && !_sound.empty())
        centered(small, 27, "Choose devices, then Save", Label);
    const auto visible = visibleIndices();
    if (_page == Page::Main) {
        bevel(8, 41, 150, 135, PanelFill, PanelShadow, PanelLight);
        const auto fact = [&](const char *key) {
            const auto it = _facts.find(key);
            return it == _facts.end() ? std::string{} : it->second;
        };
        const auto value = [&](const char *key) {
            auto s = fact(key);
            const auto p = s.find(';');
            return s.substr(0, p);
        };
        const auto line = [&](int y, const std::string &label, const std::string &shown,
                              int color) {
            text(small, 11, y, label, Label);
            const int x = 11 + width(small, label) + 6;
            text(small, x, y, fit(small, shown, 155 - x), color);
        };
        const auto below = [&](int y, const std::string &shown, int color) {
            text(small, 17, y, fit(small, shown, 138), color);
        };
        const bool usable = fact("game.status") == "usable";
        line(44, "Game:",
             usable ? (fact("game.exe").rfind("unusable", 0) == 0 ? "CD; no intro" : "CD ready")
                    : "Not usable",
             usable ? Value : Warning);
        if (!fact("cfg.error").empty()) {
            text(small, 11, 57, "Sound setup damaged", Warning);
            text(small, 11, 68, "Reset to installed", Warning);
        } else if (!fact("sound.error").empty()) {
            text(small, 11, 57, "Sound unreadable", Warning);
            text(small, 11, 68, "Open Sound setup", Warning);
        } else {
            const auto silent = [&](const char *key) {
                return fact(key).find("; silent") != std::string::npos && value(key) != "None";
            };
            text(small, 11, 57, silent("sound.music") ? "Music (silent):" : "Music:", Label);
            below(68, value("sound.music"), Value);
            text(small, 11, 81, silent("sound.speech") ? "Speech (silent):" : "Speech:", Label);
            below(92, value("sound.speech"), Value);
        }
        const bool roms = fact("roms.control").rfind("identified", 0) == 0 &&
                          fact("roms.pcm").rfind("identified", 0) == 0;
        const bool absent = fact("roms.control") == "missing" && fact("roms.pcm") == "missing";
        line(106, "MT-32 ROMs:", roms ? "Found" : absent ? "Not found" : "Unrecognized", Value);
        const bool refused = !fact("language.error").empty();
        line(120, "Language:", fact("language") + (refused ? " (refused)" : ""),
             refused ? Warning : Value);
        line(134, "Saves:", fact("saves.used-slots") + " of 10", Value);
        text(small, 11, 148, "Saves folder:", Label);
        below(160, fact("saves.folder"), Value);
        for (std::size_t i = 0; i < visible.size(); ++i) {
            const auto box = rowBox(true, int(i));
            const bool selected = visible[i] == _focus;
            if (selected) {
                bevel(box.x, box.y, box.w, box.h, SelectedFill, GoldLight, GoldDeep);
                diamond(box.x + 6, box.y + 8, 3, SelectedText);
            } else
                bevel(box.x, box.y, box.w, box.h, ButtonFill, ButtonLight, ButtonDark);
            shadowed(bold, box.x + 13, box.y + 2, fit(bold, rowAt(visible[i]).label, box.w - 16),
                     selected ? SelectedText : Value);
        }
    } else {
        bevel(7, 40, 306, 137, PanelFill, PanelShadow, PanelLight);
        for (std::size_t i = 0; i < visible.size(); ++i) {
            const auto box = rowBox(false, int(i));
            const auto row = rowAt(visible[i]);
            const bool selected = visible[i] == _focus;
            if (selected)
                edged(box.x, box.y, box.w, box.h, SelectedFill, GoldLight, GoldDeep);
            text(small, box.x + 3, box.y + 1, fit(small, row.label, box.w - 6),
                 selected ? SelectedText : row.action == "noop" ? Dim : Value);
        }
        if (_page == Page::Language) {
            text(small, 11, 113, "Russian uses the old-games.ru translation.", Label);
            text(small, 11, 126, "Recorded speech stays English.", Label);
        }
    }
    if (_notice.size() <= 50)
        centered(small, 179, _notice, Warning);
    else {
        centered(small, 177, _notice.substr(0, 50), Warning);
        centered(small, 186, shortened(_notice.substr(50), 50), Warning);
    }
    return f;
}
void XeenSetupScreen::failed(int code) {
    _play = false;
    _page = Page::Failure;
    _focus = _offset = 0;
    _error = "Exit code " + std::to_string(code) + "\n";
    std::ifstream in(xeenLogFile());
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line))
        lines.push_back(line);
    const auto start = lines.size() > 12 ? lines.size() - 12 : 0;
    for (std::size_t i = start; i < lines.size(); ++i)
        _error += lines[i] + "\n";
    if (!in && lines.empty())
        _error += "The log could not be read.";
    refresh();
}
std::optional<XeenSetupScreen::Path> XeenSetupScreen::run() { return run(windowPlacementFile()); }
std::optional<XeenSetupScreen::Path> XeenSetupScreen::run(const std::optional<Path> &placement) {
    _play = _quit = false;
    SdlWindow::rememberPlacement(placement);
    if (_page == Page::Main)
        refresh(); // The controller row reads the remembered window settings.
    SdlWindow::FrameUpdateHandler handler =
        [&](const PlayerAction &a) -> std::optional<IndexedFrame> {
        std::string clipboard;
        if (const auto *key = std::get_if<DialogKeyAction>(&a); key && key->key == 22) {
            char *s = SDL_GetClipboardText();
            if (s) {
                clipboard = s;
                SDL_free(s);
            }
        }
        dispatch(a, clipboard);
        return frame();
    };
    handler.inputContext = [&](const auto &) {
        InputContext c;
        c.contextId = _contextId;
        c.readyForAction = true;
        c.dialog = std::make_shared<const DialogInput>(input());
        return c;
    };
    handler.finished = [&] { return _play || _quit; };
    if (!SdlWindow().showInteractive(frame(), "MMModern setup", handler, [] { return true; }))
        return {};
    return _play ? std::optional<Path>{_folder} : std::nullopt;
}
} // namespace mmodern
