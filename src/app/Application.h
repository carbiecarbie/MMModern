#ifndef MMODERN_APP_APPLICATION_H
#define MMODERN_APP_APPLICATION_H

#include "games/xeen/XeenNavigation.h"

#include <filesystem>
#include <cstdint>
#include <optional>

namespace mmodern {

class Application {
public:
	int run(const std::filesystem::path &gameDirectory) const;
	int inspectMap(const std::filesystem::path &gameDirectory,
		std::uint16_t mapId = 1) const;
	int inspectParty(const std::filesystem::path &gameDirectory) const;
	int inspectEvents(const std::filesystem::path &gameDirectory,
		std::uint16_t mapId,
		std::optional<std::uint8_t> x = std::nullopt,
		std::optional<std::uint8_t> y = std::nullopt,
		std::optional<XeenDirection> direction = std::nullopt,
		bool allOnly = false) const;
	int renderMap(const std::filesystem::path &gameDirectory,
		std::uint16_t mapId = 1, int x = 9, int y = 6,
		XeenDirection direction = XeenDirection::South) const;
};

} // namespace mmodern

#endif
