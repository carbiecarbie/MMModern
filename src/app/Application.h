#ifndef MMODERN_APP_APPLICATION_H
#define MMODERN_APP_APPLICATION_H

#include "games/xeen/XeenNavigation.h"
#include "app/XeenEncounterFlow.h"
#include "app/XeenSession.h"
#include "platform/XeenWonStore.h"
#include "games/xeen/XeenMonsterFixture.h"

#include <filesystem>
#include <cstdint>
#include <optional>
#include <utility>

namespace mmodern {

struct XeenGameplayServices;
class XeenAssetSource;
class Application {
public:
 explicit Application(XeenProgramState &program, std::filesystem::path uiData = {},
  std::optional<std::filesystem::path> settingsFile = {}) : _program(program), _uiData(std::move(uiData)), _settingsFile(std::move(settingsFile)) {
  _program.cloudsWon=_settingsFile && xeenReadWon(*_settingsFile);
 }
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
	// Developer: the prepared Journey on F3 with one placed monster retargeted to
	// a chosen original species (in-memory MOB). Saving is disabled.
	int monsterFixture(const std::filesystem::path &, const XeenMonsterFixture &,
		std::optional<std::uint32_t> seed = {}) const;
	int eventFixture(const std::filesystem::path &, const XeenCamera &, std::uint32_t seed = 7) const;
	// Shared production construction; providers outlive this call. Target has
	// already been resolved/checked against the installation by gameplay().
	int playGameplay(const XeenGameplayServices &, XeenCamera,
		const std::optional<std::filesystem::path> &target, bool resume,
		XeenEncounterEntry entry = XeenEncounterEntry::Ordinary, std::optional<std::uint32_t> seed = {}) const;
private:
 int runTitle(const std::filesystem::path &,std::optional<std::uint32_t> endingScore={}) const;
 using InitialPublication=std::function<bool(const XeenSaveSnapshot &,const std::function<void()> &)>;
 XeenProgramState &_program;
 std::filesystem::path _uiData;
 std::optional<std::filesystem::path> _settingsFile;
	int gameplay(const std::filesystem::path &, XeenCamera,
		const std::optional<std::filesystem::path> &, bool resume,
		XeenEncounterEntry entry = XeenEncounterEntry::Ordinary, std::optional<std::uint32_t> seed = {},
		std::optional<XeenDifficulty> originalStart = {},const XeenSessionEntry *managed = nullptr,
		XeenAssetSource *sharedAssets = nullptr,InitialPublication publication = {},XeenSessionOutcome *outcome = nullptr,
		const XeenMonsterFixture *fixture = nullptr, const XeenCamera *eventFixture = nullptr) const;
};

} // namespace mmodern

#endif
