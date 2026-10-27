#ifndef MMODERN_XEEN_WON_STORE_H
#define MMODERN_XEEN_WON_STORE_H
#include <filesystem>
#include <optional>
namespace mmodern {
// Resolved once at application entry. Tests override MMODERN_SETTINGS_FILE.
std::optional<std::filesystem::path> xeenSettingsFile();
struct XeenSettings { bool won=false; bool darksideIntroSeen=false; };
XeenSettings xeenReadSettings(const std::filesystem::path &) noexcept;
bool xeenWriteSeen(const std::filesystem::path &, bool) noexcept;
bool xeenReadWon(const std::filesystem::path &) noexcept;
bool xeenWriteWon(const std::filesystem::path &) noexcept;
}
#endif
