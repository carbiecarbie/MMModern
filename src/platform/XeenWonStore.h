#ifndef MMODERN_XEEN_WON_STORE_H
#define MMODERN_XEEN_WON_STORE_H
#include <filesystem>
#include <optional>
#include "formats/xeen/XeenLanguage.h"
namespace mmodern {
// Resolved once at application entry. Tests override MMODERN_SETTINGS_FILE.
std::optional<std::filesystem::path> xeenSettingsFile();
// M110: language and russian_translation select the Russian layer; the
// translation folder is kept while English is chosen, so switching back needs
// no new choice. Russian requires a folder; a folder must be absolute.
struct XeenSettings {
 bool won=false; bool darksideIntroSeen=false; bool darksideWon=false; bool worldWon=false;
 XeenLanguage language=XeenLanguage::English; std::filesystem::path translation;
};
XeenSettings xeenReadSettings(const std::filesystem::path &) noexcept;
bool xeenWriteSeen(const std::filesystem::path &, bool) noexcept;
bool xeenReadWon(const std::filesystem::path &) noexcept;
enum class XeenWonKind { Clouds, Darkside, World };
bool xeenWriteWon(const std::filesystem::path &,XeenWonKind = XeenWonKind::Clouds) noexcept;
bool xeenWriteLanguage(const std::filesystem::path &,XeenLanguage,const std::filesystem::path &translation) noexcept;
}
#endif
