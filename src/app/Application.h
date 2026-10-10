#ifndef MMODERN_APP_APPLICATION_H
#define MMODERN_APP_APPLICATION_H

#include "games/xeen/XeenNavigation.h"
#include "app/XeenEncounterFlow.h"
#include "app/XeenSession.h"

#include <filesystem>
#include <cstdint>
#include <optional>
#include <utility>

namespace mmodern {

struct XeenGameplayServices;
class XeenAssetSource;
class Application {
public:
 explicit Application(std::filesystem::path uiData = {}) : _uiData(std::move(uiData)) {}
	int run(const std::filesystem::path &gameDirectory) const;
	int newGame(const std::filesystem::path &gameDirectory,
		XeenDifficulty difficulty = XeenDifficulty::Adventurer,
		std::optional<std::filesystem::path> savePath = {}) const;
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
	int loadGame(const std::filesystem::path &gameDirectory, const std::filesystem::path &savePath) const;
	int journeyRegion(const std::filesystem::path &, std::optional<std::uint32_t> seed = {},
		std::optional<std::filesystem::path> savePath = {}) const;
	// Shared production construction; providers outlive this call. Target has
	// already been resolved/checked against the installation by gameplay().
	int playGameplay(const XeenGameplayServices &, XeenCamera,
		const std::optional<std::filesystem::path> &target, bool resume,
		XeenEncounterEntry entry = XeenEncounterEntry::Ordinary, std::optional<std::uint32_t> seed = {}) const;
private:
 using InitialPublication=std::function<bool(const XeenSaveSnapshot &,const std::function<void()> &)>;
 std::filesystem::path _uiData;
	int gameplay(const std::filesystem::path &, XeenCamera,
		const std::optional<std::filesystem::path> &, bool resume,
		XeenEncounterEntry entry = XeenEncounterEntry::Ordinary, std::optional<std::uint32_t> seed = {},
		std::optional<XeenDifficulty> originalStart = {},const XeenSessionEntry *managed = nullptr,
		XeenAssetSource *sharedAssets = nullptr,InitialPublication publication = {},XeenSessionOutcome *outcome = nullptr) const;
};

} // namespace mmodern

#endif
