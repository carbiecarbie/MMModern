#ifndef MMODERN_CORE_GAME_INSTALLATION_H
#define MMODERN_CORE_GAME_INSTALLATION_H

#include <filesystem>

namespace mmodern {

enum class GameEdition {
	CloudsOfXeen,
	DarksideOfXeen,
	WorldOfXeen
};

struct GameInstallation {
	std::filesystem::path root;
	std::filesystem::path xeenArchive;
	std::filesystem::path darkArchive;
	GameEdition edition = GameEdition::CloudsOfXeen;

	bool hasXeen() const {
		return !xeenArchive.empty();
	}

	bool hasDarkside() const {
		return !darkArchive.empty();
	}
};

} // namespace mmodern

#endif
