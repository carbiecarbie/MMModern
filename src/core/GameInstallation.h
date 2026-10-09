#ifndef MMODERN_CORE_GAME_INSTALLATION_H
#define MMODERN_CORE_GAME_INSTALLATION_H
#include "core/ReadOnlyData.h"
#include <filesystem>
#include <optional>
#include <vector>
namespace mmodern {
enum class GameEdition { CloudsOfXeen, DarksideOfXeen, WorldOfXeen };
enum class XeenArchiveRole { Clouds, Darkside, Intro };
struct GameInstallation {
 std::filesystem::path root, xeenArchive, darkArchive;
 GameEdition edition=GameEdition::CloudsOfXeen;
 std::optional<ReadOnlyDataFile> cloudsData, darksideData, introData, uiModule;
 std::string sourceOrigin;
 std::vector<std::filesystem::path> protectedDirectories;
 bool hasXeen() const { return cloudsData.has_value() || !xeenArchive.empty(); }
 bool hasDarkside() const { return darksideData.has_value() || !darkArchive.empty(); }
};
// Legacy path fields are only for callers constructing plain synthetic fixtures.
ReadOnlyDataFile archiveDataFile(const GameInstallation &, XeenArchiveRole);
}
#endif
