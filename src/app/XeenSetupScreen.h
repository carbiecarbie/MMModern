#pragma once

#include "core/DialogInput.h"
#include "core/IndexedFrame.h"
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mmodern {

// The setup screen owns no game state. It is discarded before the player
// entry creates Application, and is rebuilt after a failed start.
class XeenSetupScreen {
  public:
    using Path = std::filesystem::path;
    enum class Page {
        Main,
        Browser,
        BrowserOptions,
        Sound,
        Music,
        Speech,
        Advanced,
        Language,
        Failure
    };
    struct Row {
        std::string label;
        std::string action;
    };

    XeenSetupScreen();
    std::optional<Path> run();
    std::optional<Path> run(const std::optional<Path> &placement);
    void failed(int code);
    IndexedFrame frame() const;
    DialogInput input() const;
    std::vector<Row> displayRows() const;
    void dispatch(const PlayerAction &action, const std::string &clipboard = {});
    const std::vector<Row> &rows() const { return _rows; }
    Page page() const { return _page; }
    std::size_t focus() const { return _focus; }
    const std::string &notice() const { return _notice; }
    const Path &folder() const { return _folder; }
    static std::optional<Path> resolve(const Path &path);
    static std::optional<Path>
    discover(const Path &remembered, const std::vector<Path> &roots,
             const std::function<std::optional<Path>(const Path &)> &probe);
    static std::vector<Path> subfolders(const Path &path, bool hidden,
                                        std::string *error = nullptr);
    static std::vector<Path> installRoots(const Path &home);

  private:
    friend struct XeenSetupScreenTestAccess;
    std::vector<std::size_t> visibleIndices() const;
    Row rowAt(std::size_t index) const;
    void refresh();
    void report();
    void browser(const Path &path, bool translation = false);
    void activate(std::string action);
    void saveFolder(const Path &path);
    void command(const std::vector<std::string> &arguments);
    void showSound();
    void soundHint(const std::string &kind);
    void move(int amount);
    std::map<std::string, std::string> _facts, _sound;
    std::map<std::string, std::string> _answers;
    std::vector<Row> _rows;
    Path _folder, _directory, _translation;
    std::string _path, _notice, _report, _error;
    Page _page = Page::Main;
    std::size_t _focus = 0, _offset = 0;
    std::uint64_t _contextId = 1;
    bool _hidden = false, _translationBrowser = false, _russian = false, _selectPath = false;
    bool _play = false, _quit = false;
};
} // namespace mmodern
