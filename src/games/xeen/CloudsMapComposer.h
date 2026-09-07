#ifndef MMODERN_GAMES_XEEN_CLOUDS_MAP_COMPOSER_H
#define MMODERN_GAMES_XEEN_CLOUDS_MAP_COMPOSER_H

#include "core/IndexedFrame.h"
#include "games/xeen/XeenOutdoorScene.h"

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
		std::vector<XeenObjectVisual> *objectDiagnostics = nullptr) const;
	// Execute the single, already ordered outdoor command stream.
	void drawOutdoorCommands(XeenAssetSource &assets,
		const std::vector<XeenOutdoorDrawCommand> &commands) const;
};

} // namespace mmodern

#endif
