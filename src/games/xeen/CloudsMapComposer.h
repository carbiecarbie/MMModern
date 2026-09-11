#ifndef MMODERN_GAMES_XEEN_CLOUDS_MAP_COMPOSER_H
#define MMODERN_GAMES_XEEN_CLOUDS_MAP_COMPOSER_H

#include "core/IndexedFrame.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenOutdoorScene.h"
#include <cstdint>
#include <optional>

namespace mmodern {

class XeenAssetSource;
class XeenWorld;
struct XeenCharacterRulesContext;
struct XeenPartyState;

class CloudsMapComposer {
public:
	// Optional output for skipped visuals. Present corrupt metadata/sprite data
	// still raises the 16A diagnostic exception; missing metadata permits terrain.
	IndexedFrame compose(XeenAssetSource &assets, XeenWorld &world,
		const XeenPartyState &partyState,
		const XeenCamera &camera,
		const XeenCharacterRulesContext &context,
		std::vector<XeenObjectVisual> *objectDiagnostics = nullptr,
		std::optional<std::uint64_t> ordinaryPhase = std::nullopt,
		bool *containsOrdinaryAnimation = nullptr,
		std::optional<std::uint8_t> actorFrame = std::nullopt) const;
	// Execute the single, already ordered outdoor command stream.
	void drawOutdoorCommands(XeenAssetSource &assets,
		const std::vector<XeenOutdoorDrawCommand> &commands) const;
	// Execute the single, already ordered indoor geometry/object command stream.
	void drawIndoorCommands(XeenAssetSource &assets,
		const std::vector<XeenIndoorDrawCommand> &commands) const;
	// Complete a scene replay through the same production border/UI layers.
	void drawInterfaceLayers(XeenAssetSource &assets,
		const XeenPartyState &partyState,
		const XeenCharacterRulesContext &context) const;
};

} // namespace mmodern

#endif
