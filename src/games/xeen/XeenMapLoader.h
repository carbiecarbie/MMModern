#ifndef MMODERN_GAMES_XEEN_MAP_LOADER_H
#define MMODERN_GAMES_XEEN_MAP_LOADER_H

#include "games/xeen/XeenMap.h"

#include <cstdint>

namespace mmodern {

class XeenAssetSource;

class XeenMapLoader {
public:
	// Loads immutable geometry for either an outdoor or indoor Clouds map.
	// Gameplay data (.mob/.evt) deliberately remains outside this operation.
	XeenMap loadGeometryMap(XeenAssetSource &assets, XeenMapIdentity mapId) const;

	// Loads immutable geometry from the initial Clouds container. Gameplay data
	// (.mob/.evt) deliberately remains outside the world cache.
	XeenMap loadOutdoorMap(XeenAssetSource &assets, XeenMapIdentity mapId) const;

	// Milestone 6 supports only the original Clouds Area A1, without save overlays.
	XeenMap loadAreaA1(XeenAssetSource &assets) const;
};

} // namespace mmodern

#endif
