#ifndef MMODERN_GAMES_XEEN_XEEN_GAME_FLAGS_LOADER_H
#define MMODERN_GAMES_XEEN_XEEN_GAME_FLAGS_LOADER_H

#include "games/xeen/XeenGameFlags.h"

#include <cstdint>
#include <vector>

namespace mmodern {

class XeenAssetSource;

class XeenGameFlagsLoader {
public:
	XeenGameFlags loadInitialCloudsFlags(XeenAssetSource &assets) const;
	XeenGameFlags loadFromPartyResource(
		const std::vector<std::uint8_t> &partyBytes) const;
};

} // namespace mmodern

#endif
