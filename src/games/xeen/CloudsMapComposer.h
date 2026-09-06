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
	IndexedFrame compose(XeenAssetSource &assets, XeenWorld &world,
		const XeenPartyState &partyState,
		const XeenCamera &camera,
		const XeenCharacterRulesContext &context) const;
};

} // namespace mmodern

#endif
