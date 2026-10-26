#ifndef MMODERN_XEEN_WON_STORE_H
#define MMODERN_XEEN_WON_STORE_H
#include <filesystem>
#include <optional>
namespace mmodern {
// Resolved once at application entry. Tests override MMODERN_SETTINGS_FILE.
std::optional<std::filesystem::path> xeenSettingsFile();
bool xeenReadWon(const std::filesystem::path &) noexcept;
bool xeenWriteWon(const std::filesystem::path &) noexcept;
}
#endif
